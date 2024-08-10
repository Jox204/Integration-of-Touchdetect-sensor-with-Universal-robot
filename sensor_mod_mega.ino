/**
  @file TempSensor.ino
  Modbus-Arduino Example - TempSensor (Modbus TCP using Ethernet shield)
  Copyright by André Sarmento Barbosa
  https://github.com/epsilonrt/modbus-ethernet
*/
 
#include <ModbusEthernet.h>
#include <fcs.h>
#include <yahdlc.h>

#include <cdcacm.h>
#include <usbhub.h>

//#include <pgmstrings.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>






//Modbus Registers Offsets (0-9999)
const int SENSOR_IREG_START = 100;
const int SENSOR_IREG_START2 = 120;
const int DATA_NUM=25;




class ACMAsyncOper : public CDCAsyncOper
{
public:
    uint8_t OnInit(ACM *pacm);
};

uint8_t ACMAsyncOper::OnInit(ACM *pacm)
{
    uint8_t rcode;
    // Set DTR = 1 RTS=1
    rcode = pacm->SetControlLineState(3);

    if (rcode)
    {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }

    LINE_CODING	lc;
    lc.dwDTERate	= 115200;
    lc.bCharFormat	= 0;
    lc.bParityType	= 0;
    lc.bDataBits	= 8;

    rcode = pacm->SetLineCoding(&lc);

    if (rcode)
        ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);

    return rcode;
}

USB     Usb;
//USBHub     Hub(&Usb);
ACMAsyncOper  AsyncOper;
ACM           Acm(&Usb, &AsyncOper);



//ModbusEthernet object
ModbusEthernet mb;



void setup() {
    // set the chip select pins as outputs for SPI 
    pinMode(7,OUTPUT);
    pinMode(10,OUTPUT);
    Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
      Serial.println("OSCOKIRQ failed to assert");

  while(!Acm.isReady())
     Usb.Task();

  //initially disconnect the USB-host shield
  digitalWrite(7,HIGH);
    // The media access control (ethernet hardware) address for the shield
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
    // The IP address for the shield
    byte ip[] = {194, 94, 86, 20 };   
    // Config Modbus TCP 
    mb.config(mac, ip);

    // Add SENSOR_IREG register - Use addIreg() for analog Inputs
    for(int i=0;i<DATA_NUM;++i){
      mb.addIreg(SENSOR_IREG_START+i,0);
      Serial.println(i);
    }
    // for(int i=0;i<DATA_NUM;++i){
    //   mb.addIreg(SENSOR_IREG_START2+i);
    //   Serial.println(i);
    // }
    
    
    
}



const char SERIAL_COMMAND_GET_DATA[] = {0x01}; //HDLC command to request data
const uint8_t SERIAL_COMMAND_GET_DATA_SIZE = 84;
const uint8_t DEFAULT_SENSOR_ARRAY_SIZE = 72;
const uint8_t FRAME_START_BYTE = 0x7e; //HDLC frame start flag 0x7E
const uint8_t FRAME_END_BYTE = 0X7e; //HDLC frame end flag 0x7E
void read_sensor(){
  
  
   yahdlc_control_t control = {
        .frame = YAHDLC_FRAME_DATA, // Data frame
        .seq_no = 1                  // Sequence number (0 in this case)
    };

  char data_request_frame[84]; //buffer to store encoded frame
  unsigned int data_request_frame_len; //length of encoded frame 

  //encode the data to HDLC frame 
  int ret = yahdlc_frame_data(&control, SERIAL_COMMAND_GET_DATA, sizeof(SERIAL_COMMAND_GET_DATA), data_request_frame, &data_request_frame_len);

  //check if encoding was succesful.
  if(ret !=0){
      Serial.println("not succesful");

    }
  else{
   
    Serial.println("P");
    if( Acm.isReady()) {
       uint8_t rcode;
      
        //send data request frame to the sensor.
         rcode = Acm.SndData(84, (char*)data_request_frame);
         if (rcode){
            ErrorMessage<uint8_t>(PSTR("SndData"), rcode);}
       
       
       delay(30);

        //read the sensor reply.
        uint8_t  buf[80]; //buffer to store received data
        uint16_t rcvd = 168; //size of received data
        rcode = Acm.RcvData(&rcvd, buf);
         if (rcode && rcode != hrNAK)
            ErrorMessage<uint8_t>(PSTR("Ret"), rcode);

            if( rcvd ) { //more than zero bytes received
             
              // for(uint16_t i=0; i < rcvd; i++ ) {
              //   Serial.print((uint8_t)buf[i]); //printing received data
              
              // }
              // Serial.println();
            //Ignore packages of incorrect sizes. 
            
            if(rcvd<SERIAL_COMMAND_GET_DATA_SIZE) 
                return;
            
            //Get HDLC frames from received data
            get_hdlc_frames(buf,rcvd);
            }
        delay(10);
}


  }

}


int findIndex(uint8_t array[], uint16_t size, uint8_t target,int index) {
    for (uint16_t i = index+1; i < size; ++i) {
        if (array[i] == target) {
            return (int)i; // Return the index of the found element.
        }
    }
    return -1; // 'target' not found in 'array'.
}



void get_hdlc_frames(uint8_t *data,int8_t size){
    Serial.println("A");
    int start_index=-1;   
    int stop_index=-1;
    
    while(stop_index<size-1){

      Serial.println("x");
      //Find start of frame.
      start_index = findIndex(data,size,FRAME_START_BYTE,stop_index);
      //Serial.println(start_index);
      if(start_index ==-1) 
        //No starting frame, Ignore package.
        return;
      
      //Find end of frame.  
      stop_index = findIndex(data,size,FRAME_END_BYTE,start_index);
      if(stop_index ==-1) 
        //No ending frame, Ignore package.
        return;
      //Serial.println(stop_index);
      //There's a complete frame detected. Create a new array and fill it 
      uint8_t frame_size = stop_index-start_index+1;
      uint8_t frame[frame_size];

      for(int i =0;i<frame_size;++i){
        frame[i]=data[start_index+i];
      }
      Serial.println("y");
      process_frame(frame,frame_size);
      
    }
}

void process_frame(uint8_t frame[],uint8_t frame_size){
    Serial.println("B");
    char data[84]; //buffer to store the data inside the frame
    unsigned int data_size; //buffer to store the data size

    yahdlc_control_t control;

    // for(int i=0;i<frame_size;++i){
    //   Serial.print(frame[i]); //printing the frame under process
    // }
    //Serial.println();
    //decode HDLC frame.
    int ret = yahdlc_get_data(&control,frame,frame_size,data,&data_size);
    if(ret <0){
      Serial.println("Error decoding HDLC frame");
    }
    else{

      if(control.frame==YAHDLC_FRAME_ACK){ //If the received frame is ACK frame send ACK back
        yahdlc_control_t Control = {
        .frame = YAHDLC_FRAME_ACK, // ACK frame
        .seq_no = 5                  // Sequence number 
       };
      
      char data_ack_frame[168]; //buffer to store encoded frame
      unsigned int data_ack_frame_len; //length of encoded frame 

       //encode the data to HDLC frame 
       int ret = yahdlc_frame_data(&Control, NULL, 0, data_ack_frame, &data_ack_frame_len);
       
      //  for(int i=0;i<data_ack_frame_len;i++){
      //   Serial.print((unsigned char)data_ack_frame[i]); //printing ack frame 
      //  }
      //   Serial.println();
       if(ret !=0){
      Serial.println("not succesful");

      }
       else{ Serial.println("z");
      //   Usb.Task();

      //   if( Acm.isReady()) {
      //     uint8_t rcode;

          

       
      
      //     /* sending to the phone */
      //     // rcode = Acm.SndData(84, (char*)data_ack_frame);
      //     // if (rcode){
      //     //   ErrorMessage<uint8_t>(PSTR("SndData"), rcode);}
          
      
      
      // }

       }
    }
    else{
      if(control.frame==YAHDLC_FRAME_DATA && data_size==DEFAULT_SENSOR_ARRAY_SIZE){ //if the received frame is data frame extract the data.
        //convert the data to an array form
        to_taxel_array(data,data_size);
      }
      else{
        Serial.println("Received payload with wrong size. Ignoring package.");
      }
    }
}
}


// int Taxel_Array[6][6]; //buffer to store recevied data
// //int OD[36];

// void to_taxel_array(uint8_t data[],uint8_t data_size){
//   Serial.println("c");
//   if(data_size != 72)
//     return;
//   digitalWrite(8,LOW);
//   digitalWrite(10,HIGH);
//   for(int i=0;i<6;++i){
//     for(int j=0;j<6;++j){
//       int index = (2 * i * 6)+(2 * j);
//       int value = (int)(data[index+1]*256 +data[index]);
//        Serial.print(value);
//        Serial.print(" ");
//       Taxel_Array[i][j] = value;
//       //mb.Ireg(SENSOR_IREG_START + j+6*i,value);
//     }
//     Serial.println();
//   }

// }


int Taxel_Array[6][6];
int Taxel_Array5[5][5];
void to_taxel_array(uint8_t data[],uint8_t data_size){
  //Serial.println("c");
  if(data_size != 72)
    return;
  
  for(int i=0;i<6;++i){
    for(int j=0;j<6;++j){
      int index = (2 * i * 6)+(2 * j);
      int value = (int)(data[index+1]*256 +data[index]);
        //Serial.print(value);
        //Serial.print(" ");
      Taxel_Array[i][j] = value;
    }
     //Serial.println();
  }

  for(int i=0;i<5;++i){
    for(int j=0;j<5;++j){
      
      if( j>1 && i>2)
        Taxel_Array5[i][j] = Taxel_Array[i+1][j+1];
      else
        if (j>1)
          Taxel_Array5[i][j] = Taxel_Array[i][j+1];
        else
          if(i>2)
            Taxel_Array5[i][j] = Taxel_Array[i+1][j];
          else
             Taxel_Array5[i][j] = Taxel_Array[i][j];
     
         Serial.print(Taxel_Array5[i][j]);
         Serial.print(" ");
      
    }
     Serial.println();
  }
  Serial.println();
}







int f = 0;
void loop() {
  

  if(f<2){
    digitalWrite(10,HIGH);
  digitalWrite(7,LOW);
  read_sensor();
  f=f+1;
  }
  else{
  mb.task(); // modbus action
  
  //USB-host shield on, ethernet shield off.
  digitalWrite(10,HIGH);
  digitalWrite(7,LOW);
  read_sensor();
  //USB-host shield off, ethernet shield on.
  digitalWrite(7,HIGH);
  digitalWrite(10,LOW);
  
  
  
  Serial.println(Ethernet.localIP());



  
  
  //int OD[36];
  //store sensor data in modbus input registers
  for(int i=0;i<5;++i){
    for(int j=0;j<5;++j){
      mb.Ireg(SENSOR_IREG_START + j + 5*i,Taxel_Array5[i][j]);
      //OD[j+6*i]=Taxel_Array[i][j];
    }
  }
  
  // for(int i=0;i<36;++i){
    
  //   mb.Ireg(SENSOR_IREG_START + i,OD[i]);
  //  // Serial.print(OD[i]);
  //  }
    Serial.println("here");
  // for(int i=2;i<6;++i){
  //   for(int j=4;j<6;++j){
  //     mb.Ireg(SENSOR_IREG_START2 + j + 6*i-16,Taxel_Array[i][j]);
  //   }
  // }
  //f=0;

   
  
   Serial.println("here2");
  }
      
   
}
