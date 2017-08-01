#include <NewPing.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#define TILTPIN 6
#define MAXDIST  200
#define PERIOD 100
//Note: changing MAXP may cause data loss
#define MAXP 100
#define BUFFSIZE 3000000000
bool tilts[MAXP];
int buff;
int tail=0;
int head=MAXP;
int req=0;
int last=0;
bool val;
SoftwareSerial btooth(9,10);

void setup()
{
   btooth.begin(9600);
    Serial.begin(9600);
    pinMode (TILTPIN, INPUT) ;//define the output interface tilt switch sensor
    loadFromMemory();
}
void loop()
{
  val=digitalRead(TILTPIN);
  Serial.println((val)?"TILTED":"NOT TILTED");
  writeData(val);
  if(buff==BUFFSIZE)writeToMemory();
  btooth.listen();
  Serial.println("reading command");
  readCommand();
  delay(PERIOD);
}

void writeData(bool val){
  buff++;
  tilts[tail]=val;
  if(tail==head)++head%=MAXP;
  ++tail%=MAXP;
}

void writeToMemory(){
  while(buff--){
    if(tail-buff-1>=0){
    EEPROM.update((tail-buff-1),tilts[tail-buff-1]); 
    }
    else{
    EEPROM.update((MAXP+tail-buff-1),tilts[MAXP+tail-buff-1]);
    }
  } 
  EEPROM.update(MAXP,(byte)(head>>8));
  EEPROM.update(MAXP+1,(byte)(head));
  EEPROM.update(MAXP+2,(byte)(tail>>8));      
  EEPROM.update(MAXP+3,(byte)(tail));
  EEPROM.update(MAXP+4,(byte)(last>>8));
  EEPROM.update(MAXP+5,(byte)(last));
}

int readCommand(){
int err;
if(btooth.available()>0){
  char c=btooth.read();
  Serial.print("Command type:");
  Serial.println(c);
  if(c=='u')err=updateValues();
  else if(c=='r')err=rangeValues();
  else if(c=='a')err=all();
  else if(c=='p')err=sendPeriod();
//  else if(c=='c')err=sendConfig();
  else if(c=='l')err=sendLast();
  else err=10;
  if(err==10){
    btooth.write(10);
    return -1;
  }
  return 1;
}
return 0;
}

void sendD(int err, int s, int f){
  btooth.write((byte)err);
  Serial.println("Error is: "+err);
  while(s!=f){
    btooth.write(tilts[s]);
    ++s%=MAXP;
  }
  last=f-1;
  if(last<0)last=MAXP-1;
}

int sendLast(){
  sendD(0,last,tail);
  return 0;
}

int sendPeriod(){
  btooth.write((byte)0);
  int a=PERIOD;
  byte secondbyte=a;
  a>>=8;
  byte firstbyte=a;
  btooth.write(firstbyte);
  Serial.println(firstbyte);
  btooth.write(secondbyte);
  Serial.println(secondbyte);
  return 0;
}

int updateValues(){
  if(btooth.available()>1){
    Serial.println("Updating Values");
    int a=btooth.read();
    int b=btooth.read();
    Serial.println(a);
    Serial.println(b);
    int dist=tail-head;
    if(dist<=0)dist=MAXP;
    int count=(a<<8)+b;
    Serial.print("Count:");
    Serial.println(count);
    if(count>MAXP)btooth.write((byte)2);
    else if(count>dist)btooth.write((byte)1);
    else{
      int s=tail-count;
      if(s<0)s+=MAXP;
      sendD(0,s,tail);
      Serial.print("Writing Error:");
      Serial.println((byte)0);
    //fix command
    }
    return 0;
  }
  else return 10;
}

int rangeValues(){
if(btooth.available()>3){
  int a=btooth.read();
  int b=btooth.read();
  int s=(a<<8)+b;
  a=btooth.read();
  b=btooth.read();
  int f=(a<<8)+b;
  int dist=tail-head+1;
  if(dist<=0)dist=MAXP;
  if(f<s)btooth.write((byte)3);
  else if(f>=dist)btooth.write((byte)4);
  else if(s<0||f<0)btooth.write((byte)5);
  else{
    s=(s+head)%MAXP;
    f=(f+head)%MAXP;
    sendD(0,s,f);
  }
  return 0;
}
else return 10;
}

int all(){
  int h=head;
  int t=tail;
  sendD(0,h,t);
return 0;
}

void loadFromMemory(){
    head=(EEPROM.read(MAXP))*2*2*2*2*2*2*2*2+EEPROM.read(MAXP+1);
    tail=(EEPROM.read((MAXP+2))*2*2*2*2*2*2*2*2+EEPROM.read(MAXP+3));
    last=(EEPROM.read((MAXP+4))*2*2*2*2*2*2*2*2+EEPROM.read(MAXP+5));
    int h=head;
    int t=tail;
    tail%=MAXP;
    while(h!=t){
    tilts[h]=EEPROM.read(h);
    ++h%=MAXP;
    }
}




