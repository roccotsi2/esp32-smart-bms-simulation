char hexutilConvertCharToHex(char ch)
{
  char returnType;
  switch(ch)
  {
    case '0':
    returnType = 0;
    break;
    case  '1' :
    returnType = 1;
    break;
    case  '2':
    returnType = 2;
    break;
    case  '3':
    returnType = 3;
    break;
    case  '4' :
    returnType = 4;
    break;
    case  '5':
    returnType = 5;
    break;
    case  '6':
    returnType = 6;
    break;
    case  '7':
    returnType = 7;
    break;
    case  '8':
    returnType = 8;
    break;
    case  '9':
    returnType = 9;
    break;
    case  'A':
    returnType = 10;
    break;
    case  'B':
    returnType = 11;
    break;
    case  'C':
    returnType = 12;
    break;
    case  'D':
    returnType = 13;
    break;
    case  'E':
    returnType = 14;
    break;
    case  'F' :
    returnType = 15;
    break;
    default:
    returnType = 0;
    break;
  }
  return returnType;
}

void hexutilHexStringToByteArray(byte *byteArray, String hexString, int arrayLength) {
  for(char i = 0; i < arrayLength; i++)
  {
    byte extract;
    char a = hexString[2*i];
    char b = hexString[2*i + 1];
    extract = hexutilConvertCharToHex(a)<<4 | hexutilConvertCharToHex(b);
    byteArray[i] = extract;
  }
}

void hexutilArrayToString(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}

void hexutilPrintByteArrayInHex(byte *buffer, int arrayLength) {
  char str[2*arrayLength] = "";
  hexutilArrayToString(buffer, arrayLength, str);
  Serial.println(str);
}

void hexutilSetIntValueToArray(byte *buffer, int startPosition, int value) {
  buffer[startPosition] = byte(value >> 8); // higher byte
  buffer[startPosition + 1] = byte(value & 0x00FF); // lower byte
}

int hexutilGetInteger(byte *buffer, int startPosition, int arrayLength) {
  if (startPosition > (arrayLength - 2)) {
    Serial.println("startPosition > array length");
    return 0;
  }
  int result = 0;
  result = buffer[startPosition] << 8;
  result += buffer[startPosition + 1];
  return result;
}

int16_t hexutilSwapEndian(int16_t num) {
  return (num>>8) | (num<<8);
}
