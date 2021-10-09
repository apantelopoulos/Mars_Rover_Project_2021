rov_mid_x;
rov_mid_y;
rov_angle; //of rover from its original orientation (during last time it was reset)

cam_to_obs_fwd;
cam_to_obs_side;

mid_to_cam = 75; //in mm

mid_to_obs_fwd = mid_to_cam + cam_to_obs_fwd;
mid_to_obs_side = cam_to_obs_side;

mid_to_obs_abs = sqrt(sq(mid_to_obs_fwd) + sq(mid_to_obs_side));

obs_angle_from_rov = atan(mid_to_obs_side / mid_to_obs_fwd);
obs_angle_total = rov_angle + obs_angle_from_rov;

obs_x_from_mid = mid_to_obs_abs*sin(obs_angle_total);
obs_y_from_mid = mid_to_obs_abs*cos(obs_angle_total);

obs_x = rov_mid_x + obs_x_from_mid;// do we detect cam_to_obs distances as distance to the obstacle's centre?
obs_y = rov_mid_y + obs_y_from_mid;
