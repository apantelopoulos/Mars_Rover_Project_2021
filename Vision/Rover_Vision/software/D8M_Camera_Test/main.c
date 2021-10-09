#include <stdio.h>
#include "I2C_core.h"
#include "terasic_includes.h"
#include "mipi_camera_config.h"
#include "mipi_bridge_config.h"

#include "auto_focus.h"

#include <fcntl.h>
#include <unistd.h>
#include "system.h"

//EEE_IMGPROC defines
#define GREEN_START  ('G'<<16 | 'G'<<8 | 'G')  //474747
#define BLUE_START   ('B'<<16 | 'B'<<8 | 'B')  //424242
#define PURPLE_START ('P'<<16 | 'P'<<8 | 'P')  //505050
#define ORANGE_START ('O'<<16 | 'O'<<8 | 'O')  //4f4f4f
#define BLACK_START  ('L'<<16 | 'L'<<8 | 'L')  //4c4c4c

//offsets
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG 1
#define EEE_IMGPROC_ID 2
#define EEE_IMGPROC_BBCOL 3

#define EXPOSURE_INIT 0x002000
#define EXPOSURE_STEP 0x100
#define GAIN_INIT 0x45F
#define GAIN_STEP 0x040
#define DEFAULT_LEVEL 3

#define MIPI_REG_PHYClkCtl		0x0056
#define MIPI_REG_PHYData0Ctl	0x0058
#define MIPI_REG_PHYData1Ctl	0x005A
#define MIPI_REG_PHYData2Ctl	0x005C
#define MIPI_REG_PHYData3Ctl	0x005E
#define MIPI_REG_PHYTimDly		0x0060
#define MIPI_REG_PHYSta			0x0062
#define MIPI_REG_CSIStatus		0x0064
#define MIPI_REG_CSIErrEn		0x0066
#define MIPI_REG_MDLSynErr		0x0068
#define MIPI_REG_FrmErrCnt		0x0080
#define MIPI_REG_MDLErrCnt		0x0090

void mipi_clear_error(void){
	MipiBridgeRegWrite(MIPI_REG_CSIStatus,0x01FF); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLSynErr,0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_FrmErrCnt,0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLErrCnt, 0x0000); // clear error

  	MipiBridgeRegWrite(0x0082,0x00);
  	MipiBridgeRegWrite(0x0084,0x00);
  	MipiBridgeRegWrite(0x0086,0x00);
  	MipiBridgeRegWrite(0x0088,0x00);
  	MipiBridgeRegWrite(0x008A,0x00);
  	MipiBridgeRegWrite(0x008C,0x00);
  	MipiBridgeRegWrite(0x008E,0x00);
  	MipiBridgeRegWrite(0x0090,0x00);
}

void mipi_show_error_info(void){

	alt_u16 PHY_status, SCI_status, MDLSynErr, FrmErrCnt, MDLErrCnt;

	PHY_status = MipiBridgeRegRead(MIPI_REG_PHYSta);
	SCI_status = MipiBridgeRegRead(MIPI_REG_CSIStatus);
	MDLSynErr = MipiBridgeRegRead(MIPI_REG_MDLSynErr);
	FrmErrCnt = MipiBridgeRegRead(MIPI_REG_FrmErrCnt);
	MDLErrCnt = MipiBridgeRegRead(MIPI_REG_MDLErrCnt);
	printf("PHY_status=%xh, CSI_status=%xh, MDLSynErr=%xh, FrmErrCnt=%xh, MDLErrCnt=%xh\r\n", PHY_status, SCI_status, MDLSynErr,FrmErrCnt, MDLErrCnt);
}

void mipi_show_error_info_more(void){
    printf("FrmErrCnt = %d\n",MipiBridgeRegRead(0x0080));
    printf("CRCErrCnt = %d\n",MipiBridgeRegRead(0x0082));
    printf("CorErrCnt = %d\n",MipiBridgeRegRead(0x0084));
    printf("HdrErrCnt = %d\n",MipiBridgeRegRead(0x0086));
    printf("EIDErrCnt = %d\n",MipiBridgeRegRead(0x0088));
    printf("CtlErrCnt = %d\n",MipiBridgeRegRead(0x008A));
    printf("SoTErrCnt = %d\n",MipiBridgeRegRead(0x008C));
    printf("SynErrCnt = %d\n",MipiBridgeRegRead(0x008E));
    printf("MDLErrCnt = %d\n",MipiBridgeRegRead(0x0090));
    printf("FIFOSTATUS = %d\n",MipiBridgeRegRead(0x00F8));
    printf("DataType = 0x%04x\n",MipiBridgeRegRead(0x006A));
    printf("CSIPktLen = %d\n",MipiBridgeRegRead(0x006E));
}



bool MIPI_Init(void){
	bool bSuccess;


	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_MIPI_BASE, 50*1000*1000,400*1000); //I2C: 400K
	if (!bSuccess)
		printf("failed to init MIPI- Bridge i2c\r\n");

    usleep(50*1000);
    MipiBridgeInit();

    usleep(500*1000);

//	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_CAMERA_BASE, 50*1000*1000,400*1000); //I2C: 400K
//	if (!bSuccess)
//		printf("failed to init MIPI- Camera i2c\r\n");

    MipiCameraInit();
    MIPI_BIN_LEVEL(DEFAULT_LEVEL);
//    OV8865_FOCUS_Move_to(340);

//    oc_i2c_uninit(I2C_OPENCORES_CAMERA_BASE);  // Release I2C bus , due to two I2C master shared!


 	usleep(1000);


//    oc_i2c_uninit(I2C_OPENCORES_MIPI_BASE);

	return bSuccess;
}


int green_d;
int green_h;
int purple_d;
int purple_h;
int orange_d;
int orange_h;


void is_valid(int color,int width,int ball_mid_y, int dist, int horizontal_dist){
        int width_expected = -243.5047 + 1.206599*ball_mid_y - 0.001014068*ball_mid_y*ball_mid_y;
        int width_diff = abs(width_expected - width);
        //if(width_diff<5){
            if(color==GREEN_START){
                green_d = dist;
                green_h = horizontal_dist;
            }
            if(color==PURPLE_START){
                purple_d = dist;
                purple_h = horizontal_dist;
            }
        return;
        }





bool has_changed(int color,int dist,int horizontal_dist){
	if(color==GREEN_START){
		int dist_diff=abs(dist-green_d);
		int hori_diff=abs(horizontal_dist-green_h);
		//printf("dist_diff: %d, hori_diff: %d \n",dist_diff,hori_diff); //debug
		if(dist_diff>=2 || hori_diff>=2){      //changed by more than 2, replace and send
			green_d = dist;						//old values
			green_h = horizontal_dist;
			//printf("changed \n");
			return 1;
		}
		else return 0;
	}

	if(color==PURPLE_START){
		int dist_diff=abs(dist-purple_d);
		int hori_diff=abs(horizontal_dist-purple_h);
		if(dist_diff>=2 || hori_diff>=2){
			purple_d = dist;
			purple_h = horizontal_dist;
			//printf("changed \n");
			return 1;
		}
		else return 0;
	}

	if(color==ORANGE_START){
		int dist_diff=abs(dist-orange_d);
		int hori_diff=abs(horizontal_dist-orange_h);
		if(dist_diff>=2 || hori_diff>=2){
			orange_d = dist;
			orange_h = horizontal_dist;
			//printf("changed \n");
			return 1;
		}
		else return 0;
	}


	return 0;
}



int main()
{

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

  printf("DE10-LITE D8M VGA Demo\n");
  printf("Imperial College EEE2 Project version\n");
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0x00);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0x00);

  usleep(2000);
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0xFF);
  usleep(2000);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0xFF);

  printf("Image Processor ID: %x\n",IORD(0x42000,EEE_IMGPROC_ID));
  //printf("Image Processor ID: %x\n",IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_ID)); //Don't know why this doesn't work - definition is in system.h in BSP


  usleep(2000);


  // MIPI Init
   if (!MIPI_Init()){
	  printf("MIPI_Init Init failed!\r\n");
  }else{
	  printf("MIPI_Init Init successfully!\r\n");
  }

//   while(1){
 	    mipi_clear_error();
	 	usleep(50*1000);
 	    mipi_clear_error();
	 	usleep(1000*1000);
	    mipi_show_error_info();
//	    mipi_show_error_info_more();
	    printf("\n");
//   }


#if 0  // focus sweep
	    printf("\nFocus sweep\n");
 	 	alt_u16 ii= 350;
 	    alt_u8  dir = 0;
 	 	while(1){
 	 		if(ii< 50) dir = 1;
 	 		else if (ii> 1000) dir =0;

 	 		if(dir) ii += 20;
 	 		else    ii -= 20;

 	    	printf("%d\n",ii);
 	     OV8865_FOCUS_Move_to(ii);
 	     usleep(50*1000);
 	    }
#endif






    //////////////////////////////////////////////////////////
        alt_u16 bin_level = DEFAULT_LEVEL;
        alt_u8  manual_focus_step = 10;
        alt_u16  current_focus = 300;
    	int boundingBoxColour = 0;
    	alt_u32 exposureTime = EXPOSURE_INIT;
    	alt_u16 gain = GAIN_INIT;

        OV8865SetExposure(exposureTime);
        OV8865SetGain(gain);
        Focus_Init();


        FILE *fp;
        fp = fopen ("/dev/uart", "r+");

        if(fp){printf("Opened UART\n");}
        else {printf("Failed UART\n"); while(1);}

        int count = 0;

  while(1){

       // touch KEY0 to trigger Auto focus
	   if((IORD(KEY_BASE,0)&0x03) == 0x02){

    	   current_focus = Focus_Window(320,240);
       }
	   // touch KEY1 to ZOOM
	         if((IORD(KEY_BASE,0)&0x03) == 0x01){
	      	   if(bin_level == 3 )bin_level = 1;
	      	   else bin_level ++;
	      	   printf("set bin level to %d\n",bin_level);
	      	   MIPI_BIN_LEVEL(bin_level);
	      	 	usleep(500000);

	         }


	#if 0
       if((IORD(KEY_BASE,0)&0x0F) == 0x0E){

    	   current_focus = Focus_Window(320,240);
       }

       // touch KEY1 to trigger Manual focus  - step
       if((IORD(KEY_BASE,0)&0x0F) == 0x0D){

    	   if(current_focus > manual_focus_step) current_focus -= manual_focus_step;
    	   else current_focus = 0;
    	   OV8865_FOCUS_Move_to(current_focus);

       }

       // touch KEY2 to trigger Manual focus  + step
       if((IORD(KEY_BASE,0)&0x0F) == 0x0B){
    	   current_focus += manual_focus_step;
    	   if(current_focus >1023) current_focus = 1023;
    	   OV8865_FOCUS_Move_to(current_focus);
       }

       // touch KEY3 to ZOOM
       if((IORD(KEY_BASE,0)&0x0F) == 0x07){
    	   if(bin_level == 3 )bin_level = 1;
    	   else bin_level ++;
    	   printf("set bin level to %d\n",bin_level);
    	   MIPI_BIN_LEVEL(bin_level);
    	 	usleep(500000);

       }
	#endif


int state=0;
int ymax;
int xmax;
int ymin;
int xmin;
int width;
//int height;
int dist;
int ball_mid_x;
int ball_mid_y;
//int adjusted_width;
int x_from_cent;
int horizontal_dist;
int color;
char prompt;



       //Read messages from the image processor and print them on the terminal
       while ((IORD(0x42000,EEE_IMGPROC_STATUS)>>8) & 0xff) { 	//Find out if there are words to read
           int word = IORD(0x42000,EEE_IMGPROC_MSG); 			//Get next word from message buffer
           if(state==1){xmin=word>>16; ymin=(word<<16)>>16;}
           if(state==2){xmax=word>>16; ymax=(word<<16)>>16;

           	    width = xmax-xmin;
             	//height = ymax-ymin;


             	//if(height>width){adjusted_width = height;}
             	//else {adjusted_width = width;}
             	//adjusted_width = width;
             	ball_mid_x = (xmin+xmax)/2;
             	ball_mid_y = ymin + width/2;

             	if(ymin==479 && xmin==639 && ymax==0 && xmax==0){dist = 0;}
             	else if(ball_mid_y>450){dist = 26;}  //top ball
             	//else if(ymin>403){dist=26;}
             	//else if(ball_mid_y>450){dist = 264.0004 - 1.041349*ymin + 0.001116841*ymin*ymin};
             	//else{ dist = 109.7956 - 1.584484*adjusted_width + 0.007493854*adjusted_width*adjusted_width;}
             	else{dist = 81.84514 - 0.800025*width + 0.002178674*width*width;}


             	x_from_cent = ball_mid_x -320;
             	//horizontal_dist = ( 0.1854396 - 0.002788271*adjusted_width + 0.00001360558*adjusted_width*adjusted_width)*x_from_cent;  //cm_per_pixel
             	horizontal_dist = (0.1674247 - 0.00226869*width+ 0.00000996499*width*width)*x_from_cent;

             	//mid_y -> dist
             	//if(ymin==479 && xmin==639 && ymax==0 && xmax==0){dist = 0;}
             	//else if(ymax>460){dist = 28;}
             	//else {dist = 357.1468 - 1.333912*ball_mid_y + 0.001346043*ball_mid_y*ball_mid_y;}
             	//mid y -> cm_per_pixel
             	//horizontal_dist = ( 0.6218332 - 0.002366701*ball_mid_y + 0.000002420923*ball_mid_y*ball_mid_y)*x_from_cent;


             	//top y -> dist
             	//if(ymin==479 && xmin==639 && ymax==0 && xmax==0){dist = 0;}
             	//else {dist = 450.8351 - 1.915177*ymin + 0.002162573*ymin*ymin;}
             	//top_y -> cm_per_pixel
             	//horizontal_dist = 0.7870395 - 0.003397061*ymin + 0.00000388058*ymin*ymin;

            	printf("dist: %d hori: %d mid_y: %d \n",dist,horizontal_dist,ymin);
              //   printf("green_d: %d, green_h %d \n",green_d,green_h);


             //	if(has_changed(color,dist,horizontal_dist) && is_valid(width,ball_mid_y) ){
                 	//printf("dist: %d hori: %d \n",dist,horizontal_dist);
                 	//printf("dist: %d hori: %d mid_y: %d \n",dist,horizontal_dist,ball_mid_y);
             		//printf("valid & changed " );
             		//usleep(50000);
             		//char newline = "\n";
            	is_valid(color,width,ball_mid_y,dist,horizontal_dist);
            	char esp_d[5];
                char esp_h[5];
                if(prompt == 's'){
                if(count==0){
                                 fprintf(fp,"GGG,");
                                 itoa(green_d, esp_d, 10);      //we always print green_d which would be replaced if valid
                                 itoa(green_h, esp_h, 10);
                                 count = 1;
                             }
                             else if(count==1){
                                 fprintf(fp,"PPP,");
                                 itoa(purple_d, esp_d, 10);
                                 itoa(purple_h, esp_h, 10);
                                 count = 0;
                             }

                             fprintf(fp, esp_d);
                             fprintf(fp,",");
                             fprintf(fp, esp_h);
                             fprintf(fp,"k");
                             prompt = 0;
                }


             	//	if(color==GREEN_START) {green_d = dist;  green_h = horizontal_dist;}
             	//	if(color==BLUE_START)  {blue_d = dist;   blue_h = horizontal_dist;}
             	//	if(color==PURPLE_START){purple_d = dist; purple_h = horizontal_dist;}
             	//	if(color==ORANGE_START){orange_d = dist; orange_h = horizontal_dist;}
             	//	if(color==BLACK_START) {black_d = dist;  black_h = horizontal_dist;}

           }

    		 if(word==GREEN_START || word==PURPLE_START){// || word==ORANGE_START){
    			 state=0;
    			 color = word;
    		     printf("\n");
    		 //    printf("%08x ",word);
    	   }

    	   state +=1;
       }

       //Update the bounding box colour
       boundingBoxColour = ((boundingBoxColour + 1) & 0xff);
       IOWR(0x42000, EEE_IMGPROC_BBCOL, (boundingBoxColour << 8) | (0xff - boundingBoxColour));

       //Process input commands
       int in = getchar();
       switch (in) {
       	   case 'e': {
       		   exposureTime += EXPOSURE_STEP;
       		   OV8865SetExposure(exposureTime);
       		   printf("\nExposure = %x ", exposureTime);
       	   	   break;}
       	   case 'd': {
       		   exposureTime -= EXPOSURE_STEP;
       		   OV8865SetExposure(exposureTime);
       		   printf("\nExposure = %x ", exposureTime);
       	   	   break;}
       	   case 't': {
       		   gain += GAIN_STEP;
       		   OV8865SetGain(gain);
       		   printf("\nGain = %x ", gain);
       	   	   break;}
       	   case 'g': {
       		   gain -= GAIN_STEP;
       		   OV8865SetGain(gain);
       		   printf("\nGain = %x ", gain);
       	   	   break;}
       	   case 'r': {
        	   current_focus += manual_focus_step;
        	   if(current_focus >1023) current_focus = 1023;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   printf("\nFocus = %x ",current_focus);
       	   	   break;}
       	   case 'f': {
        	   if(current_focus > manual_focus_step) current_focus -= manual_focus_step;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   printf("\nFocus = %x ",current_focus);
       	   	   break;}
       }


	   //Main loop del
	   prompt = getc(fp);
	   usleep(50000);
	  // if(prompt== 's') {count +=1;}
	   //if(count>1){count=0;}

   };

  fprintf(fp, "Closing the UART file.\n");
  fclose (fp);

  return 0;
}
