module EEE_IMGPROC(
	// global clock & reset
	clk,
	reset_n,
	
	// mm slave
	s_chipselect,
	s_read,
	s_write,
	s_readdata,
	s_writedata,
	s_address,

	// stream sink
	sink_data,
	sink_valid,
	sink_ready,
	sink_sop,
	sink_eop,
	
	// streaming source
	source_data,
	source_valid,
	source_ready,
	source_sop,
	source_eop,
	
	// conduit
	mode
	
);


// global clock & reset
input	clk;
input	reset_n;

// mm slave
input							s_chipselect;
input							s_read;
input							s_write;
output	reg	[31:0]	s_readdata;
input	[31:0]				s_writedata;
input	[2:0]					s_address;


// streaming sink
input	[23:0]            	sink_data;
input								sink_valid;
output							sink_ready;
input								sink_sop;
input								sink_eop;

// streaming source
output	[23:0]			  	   source_data;
output								source_valid;
input									source_ready;
output								source_sop;
output								source_eop;

// conduit export
input                         mode;

////////////////////////////////////////////////////////////////////////
//
parameter IMAGE_W = 11'd640;
parameter IMAGE_H = 11'd480;
parameter MESSAGE_BUF_MAX = 256;
parameter MSG_INTERVAL = 6;
parameter BB_COL_DEFAULT = 24'h00ff00;


wire [7:0]   red, green, blue, grey;
wire [7:0]   red_out, green_out, blue_out;

wire [7:0] r,g,b;

assign r = red;
assign g = green;
assign b = blue;

wire         sop, eop, in_valid, out_ready;
////////////////////////////////////////////////////////////////////////
// Detect red areas

//assign red_detect = red[7] & ~green[7] & ~blue[7];          //if red super red  
wire signed [15:0] hue, cmax, cmin, cdiff ,rmax, bmax, gmax, 
						 huetemp, g_b, b_r, r_g, cvalue,sat,thresh_p1,
						 thresh_p2,thresh_g1,thresh_g2,thresh_blue1,thresh_blue2,
						 thresh_o1,thresh_o2,thresh_black1,thresh_black2;
						
wire pvalid,gvalid,blue_valid,ovalid,black_valid,
	  pdetect,gdetect,blue_det,odetect,black_det,sat_o_p;


assign cmax = (r>g) ? ((r>b) ? r : b) : ((g>b) ? g : b);
assign cmin = (r<g) ? ((r<b) ? r : b) : ((g<b) ? g : b);
assign cdiff = cmax - cmin;


assign g_b = ((g-b)*60);///cdiff;
assign rmax = (cdiff==0) ? 0 :(g_b)/cdiff;

assign b_r = (b-r)*60;
assign gmax = (cdiff==0) ? 0 : (120 + (b_r)/cdiff);

assign r_g = (r-g)*60;
assign bmax = (cdiff==0) ? 0 : (240 + (r_g)/cdiff);


//assign sat = (cmax==0) ? 0 : ((cdiff*100)/cmax);
assign sat = (cmax==0) ? 0 : (cdiff*100);
//assign threshold = cmax*15;

assign cvalue = (cmax*100);
//assign cvalid = (sat>threshold) & (cvalue>8925);//  & (cvalue<22950);//cmax>50; //50x255  35x255 40x

assign thresh_p1	  =  cmax*70;
assign thresh_p2    =  cmax*85;
assign thresh_g1    =  cmax*20; //30 works
//assign thresh_g2    =  cmax*85;
assign thresh_blue1 =  cmax*10;
assign thresh_blue2 =  cmax*35;
assign thresh_o1	  =  cmax*60;       //80 better value?
//assign thresh_o2	  =  cmax*100;   //equal to 100
assign thresh_black1 =  cmax*30;
assign thresh_black2 = cmax*80;

//added p max 80,  remove/loewr if messes up pink 
assign pvalid     = (cvalue>=11475) & (cvalue<=21675) & (sat<=thresh_p2) ;//& (sat>=thresh_p1);//  55 , 70  & (cvalue<=17850)
assign gvalid     = (cvalue>=5100)  & (sat>=thresh_g1);//  & (sat<=thresh_g2); & (cvalue<=17850) // 30,70   

//blue sat<30,   can try 40 if black overlaps blue,  we can add sat min to black
assign blue_valid = (sat<=thresh_blue2);//(cvalue>=8925) & (cvalue<=12750)  & 
assign ovalid     = (cvalue>=15300)  & (sat>=thresh_o1);  //& (cvalue<25500)
assign black_valid = (cvalue<=10200);  //40
 
assign huetemp = (r>g) ? (r>b ? rmax : bmax) : (g>b ? gmax : bmax); 
assign hue = (huetemp>=0) ? huetemp : (huetemp+360);
//pink val lower min, higher max,,  oran lower hue ,higher val 60->80  ,sat threshp


//assign sat_o_p = (sat>(cmax*70));
//BLUE HUGE RANGE, overlaps,   if green bad hue till 160??
assign pdetect =  pvalid ? ((hue>=5 & hue<=15) |(hue>=350 & hue<=360)) :0;  
assign gdetect = gvalid ? (hue>=75 & hue<=140): 0;  //60 for looser bound   //lower hue,lower sat? 60 30
assign blue_det = 0 ? (hue>=10 & hue<=150) : 0;
assign odetect = ovalid ? (hue>=0 & hue<=65) : 0;  //0-65??
assign black_det = 0 ? ((hue>=0 & hue<=15) |(hue>=350 & hue<=360)) :0;   
//tighter bounds: hue 10, val<35/30 remove 350-360;(black)
//IF orange doesnt work(floor, and cannot change gain/ use min adjusted_width >> we can increase hue to 30/35


reg pdetect1,pdetect2,pdetect3,pdetect4,pdetect5;
reg gdetect1,gdetect2,gdetect3,gdetect4,gdetect5;
reg blue_det1,blue_det2,blue_det3,blue_det4,blue_det5;
reg odetect1,odetect2,odetect3,odetect4,odetect5;
reg black_det1,black_det2,black_det3,black_det4,black_det5;
reg signed [15:0] hue1,hue2,sat1,sat2, hue_gradient1,sat_gradient1,sat_grad_thresh1;
reg [7:0] cmax1;
wire [11:0] sat_grad_thresh;
wire [15:0] hue_gradient, sat_gradient,value_gradient;


always@(posedge clk) begin
	if(in_valid & ~sop & packet_video) begin
	pdetect5 <= pdetect4;
	pdetect4 <= pdetect3;
	pdetect3 <= pdetect2;
	pdetect2 <= pdetect1;
	pdetect1 <= pdetect;
	
	gdetect5 <= gdetect4;
	gdetect4 <= gdetect3;
	gdetect3 <= gdetect2;
	gdetect2 <= gdetect1;
	gdetect1 <= gdetect;

	blue_det5 <= blue_det4;
	blue_det4 <= blue_det3;
	blue_det3 <= blue_det2;
	blue_det2 <= blue_det1;
	blue_det1 <= blue_det;
	
	odetect5 <= odetect4;
	odetect4 <= odetect3;
	odetect3 <= odetect2;
	odetect2 <= odetect1;
	odetect1 <= odetect;
	
	black_det5 <= black_det4;
	black_det4 <= black_det3;
	black_det3 <= black_det2;
	black_det2 <= black_det1;
	black_det1 <= black_det;
	
	//hue2 <= hue1;
	//hue1 <= hue;
	
	//sat2 <= sat1;
	//sat1 <= sat;
	
	//cmax1 <= cmax;
	
	//hue_gradient1 <= hue_gradient;
	//sat_gradient1 <= sat_gradient;	
	
	//sat_grad_thresh1 <= sat_grad_thresh;
	
	end
	if(eop) begin
	pdetect1 <=0;
	pdetect2 <=0;    //edges   ,,  its carrying over to diff rows
	pdetect3 <=0;
	pdetect4 <=0;
	pdetect5 <=0;
	
	gdetect1 <=0;
	gdetect2 <=0;    
	gdetect3 <=0;
	gdetect4 <=0;
	gdetect5 <=0;

	blue_det1 <= 0;
	blue_det2 <= 0;
	blue_det3 <= 0;
	blue_det4 <= 0;
	blue_det5 <= 0;
	
	odetect1 <=0;
	odetect2 <=0;    
	odetect3 <=0;
	odetect4 <=0;
	odetect5 <=0;
	
	black_det1 <= 0;
	black_det2 <= 0;
	black_det3 <= 0;
	black_det4 <= 0;
	black_det5 <= 0;
	
	//hue2 <=0;
	//hue1 <=0;
	
	//sat2 <=0;
	//sat1 <=0;
	
	//cmax1 <=0;
	
	//hue_gradient1 <=0;
	
	//sat_gradient1 <=0;
	
	//sat_grad_thresh1 <=0;
	end
end

wire red_detect, green_detect,purple_detect,blue_detect,orange_detect,black_detect,color_detect;



//assign green_detect = ~red[7] & green[7] & ~blue[7]; 

assign purple_detect = pdetect1 & pdetect2 & pdetect3 & pdetect4 & pdetect5;

assign green_detect = gdetect1 & gdetect2 & gdetect3 & gdetect4 & gdetect5;

assign blue_detect = blue_det1 & blue_det2 & blue_det3 & blue_det4 & blue_det5;

assign orange_detect = odetect1 & odetect2 & odetect3 & odetect4 & odetect5;

assign black_detect = black_det1 & black_det2 & black_det3 & black_det4 & black_det5;


//assign hue_gradient = (hue2>hue1) ? (hue2-hue1) : (hue2-hue1);

//assign sat_gradient = (sat2>sat1) ? (sat2-hue1) : (sat2-sat1);

//assign sat_grad_thresh = cmax1 *10;

assign color_detect = purple_detect | blue_detect | green_detect | orange_detect;

assign red_detect = color_detect;// & (hue_gradient1>10) & (sat_gradient1>sat_grad_thresh1); //| blue_detect; //| green_detect;




// Find boundary of cursor box

// Highlight detected areas
//wire [23:0] red_high;
wire [23:0] blue_high,purple_high,green_high,orange_high,black_high;
assign grey = green[7:1] + red[7:2] + blue[7:2]; //Grey = green/2 + red/4 + blue/4
//assign red_high  =  red_detect ? {8'hff, 8'h0, 8'h0} : {grey, grey, grey};
assign blue_high = blue_detect ? {8'h0, 8'h0, 8'hff} : {grey, grey, grey};
assign purple_high = purple_detect ? {8'hcc, 8'h3f, 8'hb7} :  blue_high;
assign green_high = green_detect ? {8'h0, 8'hff, 8'h0} : purple_high;
assign orange_high = orange_detect ? {8'hf4, 8'h8a, 8'h2d} : green_high;
assign black_high = black_detect ? {8'h58, 8'he7, 8'hef} : orange_high;



// Show bounding box
wire [23:0] new_image;
wire bb_active;
wire purple_top_line,purple_bottom_line,purple_left_line,purple_right_line;
wire green_top_line,green_bottom_line,green_left_line,green_right_line;
wire blue_top_line,blue_bottom_line,blue_left_line,blue_right_line;
wire orange_top_line,orange_bottom_line,orange_left_line,orange_right_line;
wire black_top_line,black_bottom_line,black_left_line,black_right_line;

assign purple_top_line = (y==purple_top) & (x>purple_left & x<purple_right);
assign purple_bottom_line = (y==purple_bottom) & (x>purple_left & x<purple_right);
assign purple_left_line = (x==purple_left) & (y<purple_bottom & y>purple_top);        //y counts downwards, bottom is bigger number
assign purple_right_line = (x==purple_right) & (y<purple_bottom & y>purple_top);

assign green_top_line = (y==green_top) & (x>green_left & x<green_right);
assign green_bottom_line = (y==green_bottom) & (x>green_left & x<green_right);
assign green_left_line = (x==green_left) & (y<green_bottom & y>green_top);        //y counts downwards, bottom is bigger number
assign green_right_line = (x==green_right) & (y<green_bottom & y>green_top);

assign blue_top_line = (y==blue_top) & (x>blue_left & x<blue_right);
assign blue_bottom_line = (y==blue_bottom) & (x>blue_left & x<blue_right);
assign blue_left_line = (x==blue_left) & (y<blue_bottom & y>blue_top);        //y counts downwards, bottom is bigger number
assign blue_right_line = (x==blue_right) & (y<blue_bottom & y>blue_top);

assign orange_top_line = (y==orange_top) & (x>orange_left & x<orange_right);
assign orange_bottom_line = (y==orange_bottom) & (x>orange_left & x<orange_right);
assign orange_left_line = (x==orange_left) & (y<orange_bottom & y>orange_top);        //y counts downwards, bottom is bigger number
assign orange_right_line = (x==orange_right) & (y<orange_bottom & y>orange_top);

assign black_top_line = (y==black_top) & (x>black_left & x<black_right);
assign black_bottom_line = (y==black_bottom) & (x>black_left & x<black_right);
assign black_left_line = (x==black_left) & (y<black_bottom & y>black_top);        //y counts downwards, bottom is bigger number
assign black_right_line = (x==black_right) & (y<black_bottom & y>black_top);



assign bb_active = purple_top_line | purple_bottom_line | purple_left_line | purple_right_line | 
						 green_top_line | green_bottom_line | green_left_line | green_right_line |
						 blue_top_line | blue_bottom_line | blue_left_line | blue_right_line |
						 orange_top_line | orange_bottom_line | orange_left_line | orange_right_line |
						 black_top_line | black_bottom_line | black_left_line | black_right_line ;

assign new_image = bb_active ? bb_col : black_high;

// Switch output pixels depending on mode switch
// Don't modify the start-of-packet word - it's a packet discriptor
// Don't modify data in non-video packets
assign {red_out, green_out, blue_out} = (mode & ~sop & packet_video) ? new_image : {red,green,blue};

//Count valid pixels to tget the image coordinates. Reset and detect packet type on Start of Packet.
reg [10:0] x, y;
reg packet_video;
always@(posedge clk) begin
	if (sop) begin
		x <= 11'h0;
		y <= 11'h0;
		packet_video <= (blue[3:0] == 3'h0);
	end
	else if (in_valid) begin
		if (x == IMAGE_W-1) begin
			x <= 11'h0;
			y <= y + 11'h1;
		end
		else begin
			x <= x + 11'h1;
		end
	end
end

//Find first and last red pixels
reg [10:0] green_x_min, green_y_min, green_x_max, green_y_max;
always@(posedge clk) begin
	if (green_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < green_x_min) green_x_min <= x;
		if (x > green_x_max) green_x_max <= x;
		if (y < green_y_min) green_y_min <= y;
		green_y_max <= y;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		green_x_min <= IMAGE_W-11'h1;
		green_x_max <= 0;
		green_y_min <= IMAGE_H-11'h1;
		green_y_max <= 0;
	end
end

reg [10:0] purple_x_min, purple_y_min, purple_x_max, purple_y_max;
always@(posedge clk) begin
	if (purple_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < purple_x_min) purple_x_min <= x;
		if (x > purple_x_max) purple_x_max <= x;
		if (y < purple_y_min) purple_y_min <= y;
		purple_y_max <= y;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		purple_x_min <= IMAGE_W-11'h1;
		purple_x_max <= 0;
		purple_y_min <= IMAGE_H-11'h1;
		purple_y_max <= 0;
	end
end

reg [10:0] blue_x_min, blue_y_min, blue_x_max, blue_y_max;
always@(posedge clk) begin
	if (blue_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < blue_x_min) blue_x_min <= x;
		if (x > blue_x_max) blue_x_max <= x;
		if (y < blue_y_min) blue_y_min <= y;
		blue_y_max <= y;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		blue_x_min <= IMAGE_W-11'h1;
		blue_x_max <= 0;
		blue_y_min <= IMAGE_H-11'h1;
		blue_y_max <= 0;
	end
end

reg [10:0] orange_x_min, orange_y_min, orange_x_max, orange_y_max;
always@(posedge clk) begin
	if (orange_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < orange_x_min) orange_x_min <= x;
		if (x > orange_x_max) orange_x_max <= x;
		if (y < orange_y_min) orange_y_min <= y;
		orange_y_max <= y;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		orange_x_min <= IMAGE_W-11'h1;
		orange_x_max <= 0;
		orange_y_min <= IMAGE_H-11'h1;
		orange_y_max <= 0;
	end
end

reg [10:0] black_x_min, black_y_min, black_x_max, black_y_max;
always@(posedge clk) begin
	if (black_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < black_x_min) black_x_min <= x;
		if (x > black_x_max) black_x_max <= x;
		if (y < black_y_min) black_y_min <= y;
		black_y_max <= y;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		black_x_min <= IMAGE_W-11'h1;
		black_x_max <= 0;
		black_y_min <= IMAGE_H-11'h1;
		black_y_max <= 0;
	end
end

//Process bounding box at the end of the frame.
reg [3:0] msg_state;
reg [10:0] green_left, green_right, green_top, green_bottom;
reg [10:0] purple_left, purple_right, purple_top, purple_bottom;
reg [10:0] blue_left, blue_right, blue_top, blue_bottom;
reg [10:0] orange_left, orange_right, orange_top, orange_bottom;
reg [10:0] black_left, black_right, black_top, black_bottom;
reg [7:0] frame_count;
always@(posedge clk) begin
	if (eop & in_valid & packet_video) begin  //Ignore non-video packets
		
		//Latch edges for display overlay on next frame
		green_left <= green_x_min;
		green_right <= green_x_max;
		green_top <= green_y_min;
		green_bottom <= green_y_max;
		
		purple_left <= purple_x_min;
		purple_right <= purple_x_max;
		purple_top <= purple_y_min;
		purple_bottom <= purple_y_max;
		
		blue_left <= blue_x_min;
		blue_right <= blue_x_max;
		blue_top <= blue_y_min;
		blue_bottom <= blue_y_max;
		
		orange_left <= orange_x_min;
		orange_right <= orange_x_max;
		orange_top <= orange_y_min;
		orange_bottom <= orange_y_max;
		
		black_left <= black_x_min;
		black_right <= black_x_max;
		black_top <= black_y_min;
		black_bottom <= black_y_max;
		
		
		//Start message writer FSM once every MSG_INTERVAL frames, if there is room in the FIFO
		frame_count <= frame_count - 1;
		
		if (frame_count == 0 && msg_buf_size < MESSAGE_BUF_MAX - 3) begin
			msg_state <= 2'b01;
			frame_count <= MSG_INTERVAL-1;
		end
	end
	
	//Cycle through message writer states once started
	if (msg_state != 2'b00) msg_state <= msg_state + 2'b01;

end
	
//Generate output messages for CPU
reg [31:0] msg_buf_in; 
wire [31:0] msg_buf_out;
reg msg_buf_wr;
wire msg_buf_rd, msg_buf_flush;
wire [7:0] msg_buf_size;
wire msg_buf_empty;

`define GREEN_BOX_MSG_ID "GGG"
`define BLUE_BOX_MSG_ID "BBB"
`define PURPLE_BOX_MSG_ID "PPP"
`define ORANGE_BOX_MSG_ID "OOO"
`define BLACK_BOX_MSG_ID "LLL"

always@(*) begin	//Write words to FIFO as state machine advances
	case(msg_state)
		4'b0: begin
			msg_buf_in = 32'b0;
			msg_buf_wr = 1'b0;
		end
		4'b1: begin
			msg_buf_in = `GREEN_BOX_MSG_ID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		4'b10: begin
			msg_buf_in = {5'b0, green_x_min, 5'b0, green_y_min};	//Top left coordinate
			msg_buf_wr = 1'b1;
		end
		4'b11: begin
			msg_buf_in = {5'b0, green_x_max, 5'b0, green_y_max}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b100: begin
			msg_buf_in = `BLUE_BOX_MSG_ID; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b101: begin
			msg_buf_in = {5'b0, blue_x_min, 5'b0, blue_y_min}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b110: begin
			msg_buf_in = {5'b0, blue_x_max, 5'b0, blue_y_max}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b111: begin
			msg_buf_in = `PURPLE_BOX_MSG_ID; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1000: begin
			msg_buf_in = {5'b0, purple_x_min, 5'b0, purple_y_min}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1001: begin
			msg_buf_in = {5'b0, purple_x_max, 5'b0, purple_y_max}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1010: begin
			msg_buf_in = `ORANGE_BOX_MSG_ID; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1011: begin
			msg_buf_in = {5'b0, orange_x_min, 5'b0, orange_y_min}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1100: begin
			msg_buf_in = {5'b0, orange_x_max, 5'b0, orange_y_max}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1101: begin
			msg_buf_in = `BLACK_BOX_MSG_ID; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1110: begin
			msg_buf_in = {5'b0, black_x_min, 5'b0, black_y_min}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'b1111: begin
			msg_buf_in = {5'b0, black_x_max, 5'b0, black_y_max}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
	endcase
end


//Output message FIFO
MSG_FIFO	MSG_FIFO_inst (
	.clock (clk),
	.data (msg_buf_in),
	.rdreq (msg_buf_rd),
	.sclr (~reset_n | msg_buf_flush),
	.wrreq (msg_buf_wr),
	.q (msg_buf_out),
	.usedw (msg_buf_size),
	.empty (msg_buf_empty)
	);


//Streaming registers to buffer video signal
STREAM_REG #(.DATA_WIDTH(26)) in_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(sink_ready),
	.valid_out(in_valid),
	.data_out({red,green,blue,sop,eop}),
	.ready_in(out_ready),
	.valid_in(sink_valid),
	.data_in({sink_data,sink_sop,sink_eop})
);

STREAM_REG #(.DATA_WIDTH(26)) out_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(out_ready),
	.valid_out(source_valid),
	.data_out({source_data,source_sop,source_eop}),
	.ready_in(source_ready),
	.valid_in(in_valid),
	.data_in({red_out, green_out, blue_out, sop, eop})
);


/////////////////////////////////
/// Memory-mapped port		 /////
/////////////////////////////////

// Addresses
`define REG_STATUS    			0
`define READ_MSG    				1
`define READ_ID    				2
`define REG_BBCOL					3

//Status register bits
// 31:16 - unimplemented
// 15:8 - number of words in message buffer (read only)
// 7:5 - unused
// 4 - flush message buffer (write only - read as 0)
// 3:0 - unused


// Process write

reg  [7:0]   reg_status;
reg	[23:0]	bb_col;

always @ (posedge clk)
begin
	if (~reset_n)
	begin
		reg_status <= 8'b0;
		bb_col <= BB_COL_DEFAULT;
	end
	else begin
		if(s_chipselect & s_write) begin
		   if      (s_address == `REG_STATUS)	reg_status <= s_writedata[7:0];
		   if      (s_address == `REG_BBCOL)	bb_col <= s_writedata[23:0];
		end
	end
end


//Flush the message buffer if 1 is written to status register bit 4
assign msg_buf_flush = (s_chipselect & s_write & (s_address == `REG_STATUS) & s_writedata[4]);


// Process reads
reg read_d; //Store the read signal for correct updating of the message buffer

// Copy the requested word to the output port when there is a read.
always @ (posedge clk)
begin
   if (~reset_n) begin
	   s_readdata <= {32'b0};
		read_d <= 1'b0;
	end
	
	else if (s_chipselect & s_read) begin
		if   (s_address == `REG_STATUS) s_readdata <= {16'b0,msg_buf_size,reg_status};
		if   (s_address == `READ_MSG) s_readdata <= {msg_buf_out};
		if   (s_address == `READ_ID) s_readdata <= 32'h1234EEE2;
		if   (s_address == `REG_BBCOL) s_readdata <= {8'h0, bb_col};
	end
	
	read_d <= s_read;
end

//Fetch next word from message buffer after read from READ_MSG
assign msg_buf_rd = s_chipselect & s_read & ~read_d & ~msg_buf_empty & (s_address == `READ_MSG);
						


endmodule
