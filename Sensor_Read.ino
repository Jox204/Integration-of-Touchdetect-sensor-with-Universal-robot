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


const int led =2;


void setup()
{ 

  pinMode(10,OUTPUT);
  pinMode(led,OUTPUT);
  digitalWrite(10,HIGH);
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
      Serial.println("OSCOKIRQ failed to assert");

 
  while(!Acm.isReady()){
    Usb.Task();
    Serial.println("no");
  }

  delay( 200 );
}


const char SERIAL_COMMAND_GET_DATA[] = {0x01}; //HDLC command to request data
const uint8_t SERIAL_COMMAND_GET_DATA_SIZE = 84;
const uint8_t DEFAULT_SENSOR_ARRAY_SIZE = 72;
const uint8_t FRAME_START_BYTE = 0x7e; //HDLC frame start flag 0x7E
const uint8_t FRAME_END_BYTE = 0X7e; //HDLC frame end flag 0x7E
void on_timer_event(){
  
  Serial.println("l");
   yahdlc_control_t control = {
        .frame = YAHDLC_FRAME_DATA, // Data frame
        .seq_no = 1                  // Sequence number (0 in this case)
    };

  char data_request_frame[168]; //buffer to store encoded frame
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
      
       /* reading the keyboard */
       


      
         /* sending to the phone */
         rcode = Acm.SndData(84, (char*)data_request_frame);
         if (rcode){
            ErrorMessage<uint8_t>(PSTR("SndData"), rcode);}
       
       
       delay(30);

        /* reading the phone */
        /* buffer size must be greater or equal to max.packet size */
        /* it it set to 64 (largest possible max.packet size) here, can be tuned down
        for particular endpoint */
        uint8_t  buf[168]; //buffer to store received data
        uint16_t rcvd = 168; //size of received data
        rcode = Acm.RcvData(&rcvd, buf);
         if (rcode && rcode != hrNAK)
            ErrorMessage<uint8_t>(PSTR("Ret"), rcode);

            if( rcvd ) { //more than zero bytes received
             
              // for(uint16_t i=0; i < rcvd; i++ ) {
              //   Serial.print((unsigned char)buf[i]); //printing received data
              
              // }
              // Serial.println();
            //Ignore packages of incorrect sizes  
            
            if(rcvd<SERIAL_COMMAND_GET_DATA_SIZE)
                return;
            
            //Get HDLC frames from received data
            get_hdlc_frames(buf,rcvd);
            }
        delay(10);
    //if( Usb.getUsbTaskState() == USB_STATE_RUNNING..
}


  }

}


int findIndex(uint8_t array[], uint16_t size, uint8_t target,int index) {
    for (uint16_t i = index+1; i < size; ++i) {
        if (array[i] == target) {
            return (int)i; // Return the index of the found element
        }
    }
    return -1; // 'target' not found in 'array'
}



void get_hdlc_frames(uint8_t *data,int16_t size){
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
    char data[84];
    unsigned int data_size;

    yahdlc_control_t control;

    // for(int i=0;i<frame_size;++i){
    //   Serial.print(frame[i]); //printing the frame under process
    //   Serial.print(" ");
    // }
    // Serial.println();

    int ret = yahdlc_get_data(&control,frame,frame_size,data,&data_size);
    if(ret <0){
      Serial.println("Error decoding HDLC frame");
    }
    else{
      if(control.frame==YAHDLC_FRAME_ACK){
        yahdlc_control_t Control = {
        .frame = YAHDLC_FRAME_ACK, // Data frame
        .seq_no = 5                  // Sequence number (0 in this case)
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
      if(control.frame==YAHDLC_FRAME_DATA && data_size==DEFAULT_SENSOR_ARRAY_SIZE){
        // for(uint16_t i=0; i < data_size; i++ ) {
        //         Serial.print((unsigned char)data[i]); //printing received data
        //         Serial.print(" ");
        //       }
        //       Serial.println();
        to_taxel_array(data,data_size);
      }
      else{
        Serial.println("Received payload with wrong size. Ignoring package.");
      }
    }
}
}

int Taxel_Array[6][6];

void to_taxel_array(uint8_t data[],uint8_t data_size){
  
  if(data_size != 72)
    return;
  
  for(int i=0;i<6;++i){
    for(int j=0;j<6;++j){
      int index = (2 * i * 6)+(2 * j);
      int value = (int)(data[index+1]*256 +data[index]);
      Serial.print(value);
      Serial.print(" ");
      Taxel_Array[i][j] = value;
    }
    Serial.println();
  }

}

uint8_t start=0;



int touched(){

   for(int i =0;i<6;++i){
          for(int j=0;j<6;++j){
            if(Taxel_Array[i][j]>300)
              return 1;

            
          }
          }
      return 0;

}

void loop()
{
        //digitalWrite(led,HIGH);
        if(Serial.available()){
          Serial.read();
          if(start==0)
            start=1;
          else
            start=0;  

        }


        if(start == 1){
         
         on_timer_event();
         
          
         if(touched()){
          digitalWrite(led,HIGH);
         }
         else{
           digitalWrite(led,LOW);
         }

       
        }
  

}
