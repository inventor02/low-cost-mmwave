#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_SIZE 1024 

// Hex To Bin
void hexDigitToBinary(char hex, char *binary) {
    switch (hex) {
        case '0': strcpy(binary, "0000"); break;
        case '1': strcpy(binary, "0001"); break;
        case '2': strcpy(binary, "0010"); break;
        case '3': strcpy(binary, "0011"); break;
        case '4': strcpy(binary, "0100"); break;
        case '5': strcpy(binary, "0101"); break;
        case '6': strcpy(binary, "0110"); break;
        case '7': strcpy(binary, "0111"); break;
        case '8': strcpy(binary, "1000"); break;
        case '9': strcpy(binary, "1001"); break;
        case 'A': case 'a': strcpy(binary, "1010"); break;
        case 'B': case 'b': strcpy(binary, "1011"); break;
        case 'C': case 'c': strcpy(binary, "1100"); break;
        case 'D': case 'd': strcpy(binary, "1101"); break;
        case 'E': case 'e': strcpy(binary, "1110"); break;
        case 'F': case 'f': strcpy(binary, "1111"); break;
        default: strcpy(binary, "0000"); break;
    }
}

// Hex String To Bin
void hexToBinary(const char *hex, char *binary) {
    char temp[5]; 
    binary[0] = '\0'; 
    for (size_t i = 0; i < strlen(hex); i++) {
        hexDigitToBinary(hex[i], temp);
        strcat(binary, temp);
    }
}

// Bin String To Hex
void binaryToHex(const char *binary, char *hex) {
    int len = strlen(binary);
    int remainder = len % 4;

    char paddedBinary[MAX_SIZE * 4] = "";

    if (remainder != 0) {
        int padding = 4 - remainder;
        for (int i = 0; i < padding; i++) {
            strcat(paddedBinary, "0");
        }
    }
    strcat(paddedBinary, binary);

    char temp[5] = "";
    int index = 0;
    hex[0] = '\0';
    while (paddedBinary[index] != '\0') {
        strncpy(temp, &paddedBinary[index], 4);
        temp[4] = '\0';
        if (strcmp(temp, "0000") == 0) strcat(hex, "0");
        else if (strcmp(temp, "0001") == 0) strcat(hex, "1");
        else if (strcmp(temp, "0010") == 0) strcat(hex, "2");
        else if (strcmp(temp, "0011") == 0) strcat(hex, "3");
        else if (strcmp(temp, "0100") == 0) strcat(hex, "4");
        else if (strcmp(temp, "0101") == 0) strcat(hex, "5");
        else if (strcmp(temp, "0110") == 0) strcat(hex, "6");
        else if (strcmp(temp, "0111") == 0) strcat(hex, "7");
        else if (strcmp(temp, "1000") == 0) strcat(hex, "8");
        else if (strcmp(temp, "1001") == 0) strcat(hex, "9");
        else if (strcmp(temp, "1010") == 0) strcat(hex, "A");
        else if (strcmp(temp, "1011") == 0) strcat(hex, "B");
        else if (strcmp(temp, "1100") == 0) strcat(hex, "C");
        else if (strcmp(temp, "1101") == 0) strcat(hex, "D");
        else if (strcmp(temp, "1110") == 0) strcat(hex, "E");
        else if (strcmp(temp, "1111") == 0) strcat(hex, "F");
        index += 4;
    }
}

void hexToDec(char *hex, char *decimal) {
    int decValue = 0;

    // Iterate through each character in the hex string
    for (size_t i = 0; hex[i] != '\0'; i++) {
        if (hex[i] >= '0' && hex[i] <= '9') {
            decValue = decValue * 16 + (hex[i] - '0');
        } else if (hex[i] >= 'A' && hex[i] <= 'F') {
            decValue = decValue * 16 + (10 + (hex[i] - 'A'));
        } else if (hex[i] >= 'a' && hex[i] <= 'f') {
            decValue = decValue * 16 + (10 + (hex[i] - 'a'));
        } else {
            strcpy(decimal, "0"); // Handle invalid input by treating it as '0'
            return;
        }
		sprintf(decimal, "%d", decValue);
    }
}


// Remove Space Func
void removeSpaces(const char *input, char *output) {
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0'; i++) {
        if (input[i] != ' ' && input[i] != '\t') {
            output[j++] = input[i];
        }
    }
    output[j] = '\0'; // Null-terminate the output string
}

int main() {
    char input[MAX_SIZE];
    char noSpaces[MAX_SIZE];
    char HexNumber[MAX_SIZE * 4]; 


// Input
    printf("Enter a hexadecimal string: ");
    fgets(input, MAX_SIZE, stdin);
    input[strcspn(input, "\n")] = '\0';

    removeSpaces(input, noSpaces);
    printf("\nSync: %.8s --> ", noSpaces);
    char Sync[8];
    strncpy(Sync, &noSpaces[0], 8);
    if (strcmp(Sync, "12344321") == 0) {printf("Master to Slave\n\n");} 
    else if (strcmp(Sync, "56788765") == 0){printf("Ready To Receive\n\n");}
    else if (strcmp(Sync, "DCBAABCD") == 0){printf("Salve to Master\n\n");}
    else {printf("Unknown SYNC: %s\n\n",Sync);}
    
// Opcode
    if (strlen(noSpaces) > 8) {
        strncpy(HexNumber, &noSpaces[8], 4);
        HexNumber[4] = '\0';
		char binaryConverted[MAX_SIZE * 4];
        hexToBinary(HexNumber, binaryConverted);
        printf("Opcode: %s (%s)\n", HexNumber, binaryConverted);
// Direction
        printf("- Direction: %.4s --> ", &binaryConverted[strlen(binaryConverted) - 4]);
        char Direction[4] ;
        Direction[4] = '\0';
        strncpy(Direction, &binaryConverted[strlen(binaryConverted) - 4], 4);
        if (strcmp(Direction, "0000") == 0) {printf("Invalid\n");} 
        else if (strcmp(Direction, "0001") == 0){printf("Communication from Host to BSS\n");}
        else if (strcmp(Direction, "0010") == 0){printf("Communication from BSS to Host\n");}
        else if (strcmp(Direction, "0011") == 0){printf("Communication from Host to DSS\n");}
        else if (strcmp(Direction, "0100") == 0){printf("Communication from DSS to Host\n");}
        else if (strcmp(Direction, "0101") == 0){printf("Communication from Host to Master\n");}
        else if (strcmp(Direction, "0110") == 0){printf("Communication from Master to Host\n");}
        else if (strcmp(Direction, "0111") == 0){printf("Communication from BSS to Master\n");}
        else if (strcmp(Direction, "1000") == 0){printf("Communication from Master to BSS\n");}
        else if (strcmp(Direction, "1001") == 0){printf("Communication from BSS to DSS\n");}
        else if (strcmp(Direction, "1010") == 0){printf("Communication from DSS to BSS\n");}
        else if (strcmp(Direction, "1011") == 0){printf("Communication from Master to DSS\n");}
        else if (strcmp(Direction, "1100") == 0){printf("Communication from DSS to Master\n");}
        else if (strcmp(Direction, "1101") == 0){printf("Reserved\n");}
        else if (strcmp(Direction, "1110") == 0){printf("Reserved\n");}
        else if (strcmp(Direction, "1110") == 0){printf("Reserved\n");}
        else {printf("Unknown Direction: %s\n\n",Direction);}
		
// Message Type
        printf("- Message Type: %.2s --> ", &binaryConverted[strlen(binaryConverted) - 6]);
		char MessageType[2] ;
        MessageType[2] = '\0';
        strncpy(MessageType, &binaryConverted[strlen(binaryConverted) - 6], 2);
        if (strcmp(MessageType, "00") == 0) {printf("Command\n");} 
        else if (strcmp(MessageType, "01") == 0){printf("Response (ACK/ERROR)\n");}
        else if (strcmp(MessageType, "10") == 0){printf("NACK\n");}
        else if (strcmp(MessageType, "11") == 0){printf("ASYNC\n");}
		else {printf("Unknown MessageType: %s\n\n",MessageType);}
        
// MessageID	
        char hexConverted[MAX_SIZE];
        int lengthTemp = strlen(binaryConverted) - 6;
        char binaryConvertTemp [lengthTemp];
        for (int i = 0; i < lengthTemp ; i++){
            binaryConvertTemp[i] = binaryConverted[i];
        }
        binaryConvertTemp[lengthTemp] = '\0';
        binaryToHex(&binaryConvertTemp[0], hexConverted);
		printf("- Message ID: %.10s, 0x%s --> ", &binaryConverted[0], hexConverted);
		
		char MessageID[10] ;
        MessageID[10] = '\0';
        strncpy(MessageID, &binaryConverted[0], 10);
        if (strcmp(MessageID, "0000000000") == 0) {printf("AWR_ERROR_MSG\n");} 								//0x00
        else if (strcmp(MessageID, "0000000001") == 0){printf("RESERVED\n");} 								//0x01
        else if (strcmp(MessageID, "0000000010") == 0){printf("RESERVED\n");} 								//0x02
		else if (strcmp(MessageID, "0000000011") == 0){printf("RESERVED\n");} 								//0x03
        else if (strcmp(MessageID, "0000000100") == 0){printf("AWR_RF_STATIC_CONF_SET_MSG\n");} 			//0x04
		else if (strcmp(MessageID, "0000000101") == 0){printf("AWR_RF_STATIC_CONF_GET_MSG\n");} 			//0x05
        else if (strcmp(MessageID, "0000000110") == 0){printf("AWR_RF_INIT_MSG\n");} 						//0x06
		else if (strcmp(MessageID, "0000000111") == 0){printf("RESERVED\n");} 								//0x07
        else if (strcmp(MessageID, "0000001000") == 0){printf("AWR_RF_DYNAMIC_CONF_SET_MSG\n");} 			//0x08
		else if (strcmp(MessageID, "0000001001") == 0){printf("AWR_RF_DYNAMIC_CONF_GET_MSG\n");} 			//0x09
        else if (strcmp(MessageID, "0000001010") == 0){printf("AWR_RF_FRAME_TRIG_MSG\n");} 					//0x0A
		else if (strcmp(MessageID, "0000001011") == 0){printf("RESERVED\n");} 								//0x0B
        else if (strcmp(MessageID, "0000001100") == 0){printf("AWR_RF_ADVANCED_FEATURES_CONF_SET_MSG\n");} 	//0x0C
		else if (strcmp(MessageID, "0000001101") == 0){printf("RESERVED\n");} 								//0x0D
        else if (strcmp(MessageID, "0000001110") == 0){printf("AWR_RF_MONITORING_CONF_SET_MSG\n\n");} 		//0x0E 
		else if (strcmp(MessageID, "0000001111") == 0){printf("RESERVED\n");} 								//0x0F
        else if (strcmp(MessageID, "0000010000") == 0){printf("RESERVED\n");} 								//0x10
		else if (strcmp(MessageID, "0000010001") == 0){printf("AWR_RF_STATUS_GET_MSG\n");} 					//0x11
        else if (strcmp(MessageID, "0000010010") == 0){printf("RESERVED\n");} 								//0x12 
		else if (strcmp(MessageID, "0000010011") == 0){printf("AWR_RF_MONITORING_REPORT_GET_MSG\n");} 		//0x13
        else if (strcmp(MessageID, "0000010100") == 0){printf("RESERVED\n");} 								//0x14 
		else if (strcmp(MessageID, "0000010101") == 0){printf("RESERVED\n");} 								//0x15
        else if (strcmp(MessageID, "0000010110") == 0){printf("AWR_RF_MISC_CONF_SET_MSG\n");} 				//0x16
		else if (strcmp(MessageID, "0000010111") == 0){printf("AWR_RF_MISC_CONF_GET_MSG\n");} 				//0x17
        else if (strcmp(MessageID, "0000011000") == 0){printf("RESERVED\n");} 								//0x18
        else if (strcmp(MessageID, "0000011001") == 0){printf("RESERVED\n");} 								//0x19 
		else if (strcmp(MessageID, "0010000000") == 0){printf("AWR_RF_ASYNC_EVENT_MSG1\n");} 				//0x80
        else if (strcmp(MessageID, "0010000001") == 0){printf("AWR_RF_ASYNC_EVENT_MSG2\n");} 				//0x81
		else if (strcmp(MessageID, "1000000000") == 0){printf("AWR_DEV_RFPOWERUP_MSG\n");} 					//0x200
        else if (strcmp(MessageID, "1000000001") == 0){printf("RESERVED\n");} 								//0x201 
		else if (strcmp(MessageID, "1000000010") == 0){printf("AWR_DEV_CONF_SET_MSG\n");} 					//0x202
        else if (strcmp(MessageID, "1000000011") == 0){printf("AWR_DEV_CONF_GET_MSG\n");} 					//0x203
		else if (strcmp(MessageID, "1000000100") == 0){printf("AWR_DEV_FILE_DONWLOAD_MSG\n");} 				//0x204
        else if (strcmp(MessageID, "1000000101") == 0){printf("RESERVED\n");} 								//0x205
		else if (strcmp(MessageID, "1000000110") == 0){printf("AWR_DEV_FRAME_CONFIG_APPLY_MSG\n");}			//0x206
        else if (strcmp(MessageID, "1000000111") == 0){printf("AWR_DEV_STATUS_GET_MSG\n");} 				//0x207
		else if (strcmp(MessageID, "1000001000") == 0){printf("RESERVED\n");} 								//0x208
        else if (strcmp(MessageID, "1000001001") == 0){printf("RESERVED\n");} 								//0x209 
		else if (strcmp(MessageID, "1000001010") == 0){printf("RESERVED\n");} 								//0x20A
        else if (strcmp(MessageID, "1000001011") == 0){printf("RESERVED\n");} 								//0x20B
		else if (strcmp(MessageID, "1000001100") == 0){printf("RESERVED\n");} 								//0x20C
        else if (strcmp(MessageID, "1000001101") == 0){printf("RESERVED\n");} 								//0x20D
		else if (strcmp(MessageID, "1010000000") == 0){printf("AWR_DEV_ASYNC_EVENT_MSG\n");} 				//0x280
		else {printf("Unknown MessageID: %s\n\n", MessageID);}
    }
	printf("\n");

// Length
    if (strlen(noSpaces) > 12) {
        strncpy(HexNumber, &noSpaces[12], 4);
        HexNumber[4] = '\0';
        char binaryConverted[MAX_SIZE * 4];
        hexToBinary(HexNumber, binaryConverted);
		char decimalConverted[5];
		hexToDec(HexNumber, decimalConverted);
		printf("Length: %s, in binary: %s\n", HexNumber, binaryConverted);
        printf("%s bytes\n\n", decimalConverted);
    }

// FLAGS
    if (strlen(noSpaces) > 16) {
        strncpy(HexNumber, &noSpaces[16], 4);
        HexNumber[4] = '\0';
        char binaryConverted[MAX_SIZE * 4];
        hexToBinary(HexNumber, binaryConverted);
        printf("FLAGS: %s (%s)\n", HexNumber, binaryConverted);
// Retry
		char Retry[2];
		Retry[2] = '\0';
		strncpy(Retry, &binaryConverted[strlen(binaryConverted) - 2], 2);
        printf("- Retry: %s --> ", Retry);
		if (strcmp(Retry, "00") == 0) {printf("New Message\n");} 
        else if (strcmp(Retry, "01") == 0){printf("RESERVED\n");}
        else if (strcmp(Retry, "10") == 0){printf("RESERVED\n");}
        else if (strcmp(Retry, "11") == 0){printf("Retransmitted Message\n");}
		else {printf("Unknown Retry: %s\n\n",Retry);}
// ACKREQ
		char ACKREQ[2];
		ACKREQ[2] = '\0';
		strncpy(ACKREQ, &binaryConverted[strlen(binaryConverted) - 4], 2);
        printf("- ACKREQ: %s --> ", ACKREQ);
		if (strcmp(ACKREQ, "00") == 0) {printf("ACK is required\n");} 
        else if (strcmp(ACKREQ, "01") == 0){printf("RESERVED\n");}
        else if (strcmp(ACKREQ, "10") == 0){printf("RESERVED\n");}
        else if (strcmp(ACKREQ, "11") == 0){printf("ACK is not required\n");}
		else {printf("Unknown ACKREQ: %s\n\n",ACKREQ);}
// Protocol VER
		char ver[4];
		ver[4] = '\0';
		strncpy(ver, &binaryConverted[strlen(binaryConverted) - 8], 4);
        printf("- Version No: %s \n", ver);

// CRCREQ
		char CRCREQ[2];
		CRCREQ[2] = '\0';
		strncpy(CRCREQ, &binaryConverted[6], 2);
        printf("- CRCREQ: %s --> ", CRCREQ);
		if (strcmp(CRCREQ, "00") == 0) {printf("CRC is appended\n");} 
        else if (strcmp(CRCREQ, "01") == 0){printf("RESERVED\n");}
        else if (strcmp(CRCREQ, "10") == 0){printf("RESERVED\n");}
        else if (strcmp(CRCREQ, "11") == 0){printf("CRC is not appended\n");}
		else {printf("Unknown CRCREQ: %s\n\n",CRCREQ);}		
// CRCLen
		char CRCLen[2];
		CRCLen[2] = '\0';
		strncpy(CRCLen, &binaryConverted[4], 2);
        printf("- CRCLen: %s --> ", CRCLen);
		if (strcmp(CRCLen, "00") == 0) {printf("16-bit\n");} 
        else if (strcmp(CRCLen, "01") == 0){printf("32-bit\n");}
        else if (strcmp(CRCLen, "10") == 0){printf("64-bit\n");}
        else if (strcmp(CRCLen, "11") == 0){printf("RESERVED\n");}
		else {printf("Unknown CRCLen: %s\n\n",CRCLen);}
// SEQNUM
		char SEQNUM[4];
		SEQNUM[4] = '\0';
		strncpy(SEQNUM, &binaryConverted[0], 4);
        printf("- SEQNUM: %s \n\n", SEQNUM);	
    }
	

// REMCHUNKS
    if (strlen(noSpaces) > 20) {
        printf("REMCHUNKS: %.4s\n\n", &noSpaces[20]);
    }

// NSBC
    if (strlen(noSpaces) > 24) {
		
		char NSBC [4];
		strncpy(NSBC, &noSpaces[24], 4);
        NSBC[4] = '\0';
		char NSBCinDEC[5];
		hexToDec(NSBC, NSBCinDEC);
		printf("Number of Subblock: %s, in decimal: %s\n\n", NSBC, NSBCinDEC);
    }

// CHKSUM
    if (strlen(noSpaces) > 28) {
        printf("CHKSUM: %.4s\n\n", &noSpaces[28]);
    }

// MSGDATA
//SubblockID
    if (strlen(noSpaces) > 32) {
        printf("SubblockID: %.4s\n\n", &noSpaces[32]);
    }
	
// SubblockLen
    if (strlen(noSpaces) > 36) {
		char SBLKLEN [4];
		strncpy(SBLKLEN, &noSpaces[36], 4);
        SBLKLEN[4] = '\0';
		char SBLKLENinDEC[5];
		hexToDec(SBLKLEN, SBLKLENinDEC);
        printf("SubblockLEN: %s, in decimal: %s\n\n", SBLKLEN, SBLKLENinDEC);
    }
	
//SubblockData + CRC
    if (strlen(noSpaces) > 40) {
		strncpy(HexNumber, &noSpaces[40], MAX_SIZE * 4);
        HexNumber[MAX_SIZE * 4] = '\0';
		char Data[MAX_SIZE * 4];
        hexToBinary(HexNumber, Data);
        printf("SubblockData + CRC: %s\n", &noSpaces[40]);
		printf("in binary: %s\n", Data);
    }

   return 0;
}
