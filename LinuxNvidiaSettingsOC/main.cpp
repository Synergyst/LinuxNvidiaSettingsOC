#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include "edid.h"
#include "xorgfiles.h"
#define MAXCARDCNT 32
#define MAX_FAN_CNT 3
#define ID_LEN 12
#define CMDBUFSIZ 65535
//#define CMDBUFSIZ 1024

/* 
 * =================================
 * Linux nvidia-settings OC utility
 * Developed by Pawkow
 * =================================
 * Forgive the shoddiness of my programming.. 
 * I aim for function, not prettiness. This is only a free tool. ;)
 * =================================
 * 
 * CHANGEME: Assume(for now) the user's X-display is :0
 * So --ctrl-display is currently hard-coded
 * 
 * NOTE: Absolute core clock and power limit is already supported natively by Nvidia-SMI/T-Rex.. it will not be implemented here.
 * 
 * TODO: Implement limits on what the user can adjust via this wrapper program.
 * Obviously Nvidia has hard limits, though we should handle this as opposed to having an error from nvidia-settings spew out into the console or something. ;)
 * Currently I only implemented a limit for the fan min/max speed.
 * 
 * TODO: Print card names along with ID/bus information.
 * 
 * FIXME: Allocate memory properly. Do not use magic numbers/magic numbers which allocate just over enough memory.
 */

int selectedCardID;
int fanVal = -1;
int lockCclockVal = 0;
int lockMclockVal = 0;
int cclockVal = 0;
int mclockVal = 0;
bool usingCardID = false;
char **lineArr;
char **lineDecArr;
char **fanArr;
int fanIter = 0;
char cOut[CMDBUFSIZ];

bool strStartsWithStr(const char* a, const char* b) {
  if (strncmp(a, b, strlen(b)) == 0) return 1;
  return 0;
}

void copyFile(char* src_path, char* dst_path) {
  // TODO: Fix file permissions if a backup file does not exist..
  int src_fd, dst_fd, n, err;
  unsigned char buffer[4096];
  src_fd = open(src_path, O_RDONLY);
  dst_fd = open(dst_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  while (1) {
    err = read(src_fd, buffer, 4096);
    if (err == -1) {
      printf("Error reading file.\n");
      exit(1);
    }
    n = err;
    if (n == 0) break;
    err = write(dst_fd, buffer, n);
    if (err == -1) {
      printf("Error writing to file.\n");
      exit(1);
    }
  }
  close(src_fd);
  close(dst_fd);
}

int runCommand(char *inCommand) {
  FILE* fpipe;
  /*strcpy(command, "lspci -nn -s ");
  strcat(command, selectedBusID);*/
  //printf("CMD: %s\n", command);
  char ch = 0;
  if (0 == (fpipe = (FILE*)popen(inCommand, "r"))) {
    perror("popen() failed.");
    exit(EXIT_FAILURE);
  }
  while (fread(&ch, sizeof ch, 1, fpipe)) {
    sprintf(cOut, "%s%c", cOut, ch);
  }
  pclose(fpipe);
  return 0;
}

int numOfCards() {
  char cmd[60];
  strcpy(cmd, "nvidia-xconfig --query-gpu-info | awk 'NR==1 { print $4 }'");
  runCommand(cmd);
  //printf("CMD: %s\n%s", cmd, cOut);
  return atoi(cOut);
}

int fan(int id, int x) {
  printf("Setting GPU#%d fan(s) to: %d%\n", id, x);
  char* cmd;
  strcpy(cOut, "");
  cmd = (char*)malloc(65535 * sizeof(char));
  sprintf(cmd, "nvidia-settings --ctrl-display=:0 -a [gpu:%d]/GPUPowerMizerMode=1 -a [gpu:%d]/GPUFanControlState=1", id, id);

  for (int c = 0; c < fanIter; c++) {
    if (atoi(fanArr[c]) == selectedCardID) {
      sprintf(cmd, "%s -a [fan:%d]/GPUTargetFanSpeed=%d", cmd, c, x);
    }
  }
  sprintf(cmd, "%s &", cmd);
  runCommand(cmd);
  free(cmd);
  strcpy(cOut, "");
  return 0;
}

int lockCclock(int id, int x) {
  printf("Locking GPU#%d core clock to: %d%\n", id, x);
  //printf("%s cclock: %d\n", str, x);
  strcpy(cOut, "");
  char cmd[36];
  sprintf(cmd, "sudo nvidia-smi -i %d -lgc %d", id, x);
  runCommand(cmd);
  strcpy(cOut, "");
  return 0;
}

int lockMclock(int id, int x) {
  printf("Locking GPU#%d memory clock to: %d%\n", id, x);
  //printf("%s cclock: %d\n", str, x);
  strcpy(cOut, "");
  char cmd[36];
  sprintf(cmd, "sudo nvidia-smi -i %d -lmc %d", id, x);
  runCommand(cmd);
  strcpy(cOut, "");
  return 0;
}

int cclock(int id, int x) {
  printf("Setting GPU#%d core clock offset to: %d%\n", id, x);
  //printf("%s cclock: %d\n", str, x);
  strcpy(cOut, "");
  char cmd[200];
  sprintf(cmd, "nvidia-settings --ctrl-display=:0 -a [gpu:%d]/GPUPowerMizerMode=1 -a [gpu:%d]/GPUGraphicsClockOffset[2]=%d -a [gpu:%d]/GPUGraphicsClockOffset[3]=%d", id, id, x, id, x);
  sprintf(cmd, "%s &", cmd);
  runCommand(cmd);
  //printf("%s", cOut);
  strcpy(cOut, "");
  return 0;
}

int resetAllClocks(int id) {
  printf("Resetting GPU#%d clocks to stock.\n", id);
  //printf("%s cclock: %d\n", str, x);
  strcpy(cOut, "");
  char cmd[36];
  sprintf(cmd, "sudo nvidia-smi -i %d -rgc", id);
  runCommand(cmd);
  strcpy(cmd, "");
  sprintf(cmd, "sudo nvidia-smi -i %d -rmc", id);
  runCommand(cmd);
  //printf("%s", cOut);
  strcpy(cOut, "");
  return 0;
}

int mclock(int id, int x) {
  printf("Setting GPU#%d memory clock offset to: %d%\n", id, x);
  //printf("%s mclock: %d\n", str, x);
  strcpy(cOut, "");
  char cmd[148];
  sprintf(cmd, "nvidia-settings --ctrl-display=:0 -a [gpu:%d]/GPUPowerMizerMode=1 -a [gpu:%d]/GPUMemoryTransferRateOffsetAllPerformanceLevels=%d", id, id, x);
  sprintf(cmd, "%s &", cmd);
  runCommand(cmd);
  //printf("%s", cOut);
  strcpy(cOut, "");
  return 0;
}

bool is_realnumber(char* instring) {
  if (*instring != '-' && *instring != '.' && !isdigit(*instring)) return false;
  if (strspn(instring + 1, "0123456789.") < strlen(instring + 1)) return false;
  int c = 0;
  while (*instring) if (*instring++ == '.') if (++c > 1) return false;
  return true;
}

int main(int argc, char *argv[]) {
  strcpy(cOut, "");
  int opt, numOfCardsVal;
  numOfCardsVal = numOfCards();
  strcpy(cOut, "");
  bool printHelp = false;
  bool resetClocks = false;

  while ((opt = getopt(argc, argv, ":f:c:m:i:l:v:rh")) != -1) {
    switch (opt) {
    case 'h':
      printHelp = true;
      break;
    case 'r':
      resetClocks = true;
      break;
    case 'i':
      if ((!is_realnumber(optarg) && ((atoi(optarg) < (numOfCardsVal - 1)) && (atoi(optarg) > -1)))) {
        printf("%s is not a valid card ID, exiting..\n", optarg);
        return 1;
      }
      usingCardID = true;
      selectedCardID = atoi(optarg);
      strcpy(cOut, "");
      break;
    case 'f':
      if (!is_realnumber(optarg)) {
        printf("'%s' is not a valid fan speed, exiting..\n", argv[optind-1]);
        return 1;
      }
      fanVal = atoi(optarg);
      break;
    case 'l':
      if (!is_realnumber(optarg)) {
        printf("'%s' is not a valid absolute core clock value, exiting..\n", argv[optind - 1]);
        return 1;
      }
      lockCclockVal = atoi(optarg);
      break;
    case 'v':
      if (!is_realnumber(optarg)) {
        printf("'%s' is not a valid absolute memory clock value, exiting..\n", argv[optind - 1]);
        return 1;
      }
      lockMclockVal = atoi(optarg);
      break;
    case 'c':
      if (!is_realnumber(optarg)) {
        printf("'%s' is not a valid core offset value, exiting..\n", argv[optind - 1]);
        return 1;
      }
      cclockVal = atoi(optarg);
      break;
    case 'm':
      if (!is_realnumber(optarg)) {
        printf("'%s' is not a valid memory offset value, exiting..\n", argv[optind - 1]);
        return 1;
      }
      mclockVal = atoi(optarg);
      break;
    case '?':
      printf("FATAL: Unrecognized parameter: %s\n", argv[optind - 1]);
      return 1;
      break;
    case ':':
      printf("FATAL: No value entered for: %s\n", argv[optind - 1]);
      return 1;
      break;
    default:
      printf("Exiting..\n");
      return 1;
      break;
    }
  }

  FILE* fStr;
  fStr = fopen("edid.bin", "wb");
  fwrite(&edidfile, sizeof(unsigned char), edidfsize, fStr);
  fclose(fStr);
  fStr = fopen("Xwrapper.config", "wb");
  fwrite(&xwrapperconfigfile, sizeof(unsigned char), xwrapperconfigfsize, fStr);
  fclose(fStr);

  /*copyFile("/etc/X11/Xwrapper.config", "Xwrapper.config.bak");
  printf("Backed up currently active Xwrapper.config file to Xwrapper.config.bak\nNOTE: If you have issues please move Xwrapper.config to /etc/X11/\n\n");*/

  fanArr = (char**)malloc((numOfCardsVal * MAX_FAN_CNT) * sizeof(char*));
  for (int i = 0; i < (numOfCardsVal * MAX_FAN_CNT); i++) {
    fanArr[i] = (char*)malloc((ID_LEN + 1) * sizeof(char));
    //strcpy(lineDecArr[i], your_string[i]);
  }
  strcpy(cOut, "");
  char cmdFan[128];
  strcpy(cmdFan, "nvidia-settings --ctrl-display=:0 -q fans --verbose|grep -E 'gpu:'|awk '{ print $1 }'|cut -d':' -f3|cut -d']' -f1");
  runCommand(cmdFan);
  //printf("CMD: %s\n%s", cmdFan, cOut);

  const char fanDelim[] = "\n";
  char* fanToken = strtok(cOut, fanDelim);
  // walk through other tokens
  while (fanToken != NULL) {
    strcpy(fanArr[fanIter], fanToken);
    //printf("GPU#%s, fan: %d\n", fanArr[fanIter], fanIter);
    fanToken = strtok(NULL, fanDelim);
    fanIter++;
  }

  lineArr = (char**)malloc(numOfCardsVal * sizeof(char*));
  for (int i = 0; i < numOfCardsVal; i++) {
    lineArr[i] = (char*)malloc((ID_LEN + 1) * sizeof(char));
    //strcpy(lineDecArr[i], your_string[i]);
  }
  strcpy(cOut, "");
  char cmdLineArr[128];
  strcpy(cmdLineArr, "nvidia-xconfig --query-gpu-info|grep -E 'PCI BusID'|cut -d' ' -f6|awk -F'[:]' '{ printf \"%02x:%02x.%x\\n\", $2, $3, $4 }'");
  runCommand(cmdLineArr);
  const char delim[] = "\n";
  char* token = strtok(cOut, delim);
  // walk through other tokens
  int iter = 0;
  while (token != NULL) {
    strcpy(lineArr[iter], token);
    //printf("GPU#%d, bus(hex): %s\n", iter, lineArr[iter]);
    token = strtok(NULL, delim);
    iter++;
  }

  printf("Card list: \n");
  lineDecArr = (char**)malloc(numOfCardsVal * sizeof(char*));
  for (int i = 0; i < numOfCardsVal; i++) {
    lineDecArr[i] = (char*)malloc((ID_LEN + 1) * sizeof(char));
    //strcpy(lineDecArr[i], your_string[i]);
  }
  strcpy(cOut, "");
  char cmdLineDecArr[128];
  strcpy(cmdLineDecArr, "nvidia-xconfig --query-gpu-info|grep -E 'PCI BusID'|cut -d' ' -f6|awk -F'[:]' '{ printf \"%d:%d.%d\\n\", $2, $3, $4 }'");
  runCommand(cmdLineDecArr);
  const char decDelim[] = "\n";
  char* decToken = strtok(cOut, decDelim);
  // walk through other tokens
  int decIter = 0;
  while (decToken != NULL) {
    strcpy(lineDecArr[decIter], decToken);
    //printf("GPU#%d, bus(dec): %s\n", decIter, lineArr[decIter]);
    printf("\t+---> GPU#%d, bus(hex): %s, bus(dec): %s\n", decIter, lineArr[decIter], lineDecArr[decIter]);
    decToken = strtok(NULL, decDelim);
    decIter++;
  }

  fStr = fopen("xorg.conf", "wb");
  for (int k = 0; k < xorg_conf_ref_1_fsize; k++) {
    fprintf(fStr, "%c", xorg_conf_ref_1_file[k]);
  }
  // Intentionally left out of hex char array so the user may edit the positioning of the screen here
  for (int s = 0; s < numOfCardsVal; s++) {
    fprintf(fStr, "    Screen      %d  \"Screen%d\" 0 0\n", s, s);
  }
  for (int k = 0; k < xorg_conf_ref_2_fsize; k++) {
    fprintf(fStr, "%c", xorg_conf_ref_2_file[k]);
  }
  for (int s = 0; s < numOfCardsVal; s++) {
    for (int k = 0; k < xorg_conf_ref_2_1_fsize; k++) {
      fprintf(fStr, "%c", xorg_conf_ref_2_1_file[k]);
    }
    fprintf(fStr, "%d", s);
    for (int k = 0; k < xorg_conf_ref_2_2_fsize; k++) {
      fprintf(fStr, "%c", xorg_conf_ref_2_2_file[k]);
    }
    fprintf(fStr, "%s", lineDecArr[s]);
    for (int k = 0; k < xorg_conf_ref_3_fsize; k++) {
      fprintf(fStr, "%c", xorg_conf_ref_3_file[k]);
    }
    for (int k = 0; k < xorg_conf_ref_3_1_fsize; k++) {
      fprintf(fStr, "%c", xorg_conf_ref_3_1_file[k]);
    }
    fprintf(fStr, "%d", s);
    for (int k = 0; k < xorg_conf_ref_3_2_fsize; k++) {
      fprintf(fStr, "%c", xorg_conf_ref_3_2_file[k]);
    }
    fprintf(fStr, "%d", s);
    for (int k = 0; k < xorg_conf_ref_3_3_fsize; k++) {
      fprintf(fStr, "%c", xorg_conf_ref_3_3_file[k]);
    }
    if (s == 0) {
      fprintf(fStr, "%d", s);
      for (int k = 0; k < xorg_conf_ref_3_4_fsize; k++) {
        fprintf(fStr, "%c", xorg_conf_ref_3_4_file[k]);
      }
    } else {
      for (int k = 0; k < xorg_conf_ref_3_4b_fsize; k++) {
        fprintf(fStr, "%c", xorg_conf_ref_3_4b_file[k]);
      }
      fprintf(fStr, "%d", s);
      for (int k = 0; k < xorg_conf_ref_3_4b_1_fsize; k++) {
        fprintf(fStr, "%c", xorg_conf_ref_3_4b_1_file[k]);
      }
      fprintf(fStr, "%d", s);
      for (int k = 0; k < xorg_conf_ref_3_4b_2_fsize; k++) {
        fprintf(fStr, "%c", xorg_conf_ref_3_4b_2_file[k]);
      }
      //fprintf(fStr, "%d", s);
    }
  }
  fclose(fStr);
  printf("\nGenerated xorg.conf, Xwrapper.config, and edid.bin\nEdit xorg.conf to enable fully-headless mode (default config requires a monitor plugged into the primary/first card!).\n\n");

  if (printHelp) {
    printf("Usage:              %s -f <fan percentage> -l <absolute core clock> -m <memory clock offset> -i <card ID>\n", argv[0]);
    printf("Usage(alt example): %s -f <fan percentage> -c <core clock offset> -m <memory clock offset> -i <card ID>\n", argv[0]);
    printf("\t-f\tFan percentage (ie: 0 to 100)\n\t\t    If you do not want to set a fan speed use: -f -1\n");
    printf("\t-c\tCore clock offset\n");
    printf("\t-m\tMemory clock offset\n");
    printf("\t-l\tLock core clock\n");
    printf("\t-v\tLock memory clock\n");
    printf("\t-r\tReset core/memory clocks (for supplied card ID)\n");
    printf("\t-i\tCard ID (ie: the value for GPU#0 would be 0)\n\t\t    You can get a list of card ID's associated to card names by running: nvidia-xconfig --query-gpu-info\n\n");
    printf("\t-h\tPrint help, list card ID's, and automatically generate Xorg config files\n");
    printf("\nNOTES:\n\tLocked/absolute values are mutually exclusive to their offset-method counterpart (do not try to lock your core clock and set a core clock offset).\n\n");
    printf("\tReset clocks before adjusting any cards which *already* have any clocks set (usage: %s -i <card ID> -r)\n", argv[0]);
    char username[256];
    int userRetVal = getlogin_r(username, 256);
    char nvidiaSmiLocCmd[42];
    strcpy(cOut, "");
    strcpy(nvidiaSmiLocCmd, "whereis -b nvidia-smi|awk '{ print $2 }'");
    runCommand(nvidiaSmiLocCmd);
    printf("\tAdd the following to the bottom of /etc/sudoers (required for Nvidia-SMI functionality): %s ALL = (root) NOPASSWD: %s\n\n", username, cOut);
    printf("Please report issues/suspected bugs to the GitHub page: https://github.com/Synergyst/LinuxNvidiaSettingsOC/issues\n");
    return 0;
  }

  if (selectedCardID > (numOfCardsVal - 1) || selectedCardID < 0) {
    printf("%d is not a valid card ID, exiting..\n", selectedCardID);
    return 1;
  }

  if ((cclockVal != 0) && (lockCclockVal != 0)) {
    printf("Can't lock core clock and set a core clock offset, pick one or the other.\nExiting..\n", selectedCardID);
    return 1;
  }

  if ((fanVal >= -1) && (fanVal <= 100)) {
    if (fanVal == -1) {
      printf("Will not set fan speed\n");
    } else {
      fan(selectedCardID, fanVal);
    }
  } else {
    printf("FATAL: can't set fan speed to %d%, exiting..\n", fanVal);
    return 1;
  }

  if (resetClocks) {
    resetAllClocks(selectedCardID);
  } else {
    printf("Will not reset clocks\n");
  }

  if (lockCclockVal != 0) { // FIXME: Add limits according to hardware limits per-card
    lockCclock(selectedCardID, lockCclockVal);
  } else {
    printf("Will not set core clock offset\n");
  }

  if (cclockVal != 0) { // FIXME: Add limits according to hardware limits per-card
    cclock(selectedCardID, cclockVal);
  } else {
    printf("Will not set core clock offset\n");
  }

  if (mclockVal != 0) { // FIXME: Add limits according to hardware limits per-card
    mclock(selectedCardID, mclockVal);
  } else {
    printf("Will not set memory clock offset\n");
  }

  return 0;
}