% Kalman Filter - Roll & Pitch (MPU6050)
% From uart State: [roll, pitch, bias_gx, bias_gy]
clear all; clc; close all;

% If not installed package:
% pkg install "https://github.com/gnu-octave/instrument-control/releases/download/release-0.10.1/instrument-control-0.10.1.tar.gz"
% later this package
pkg load instrument-control


%AX:-98 AY:-412 AZ:-16672 GX:-216 GY:42 GZ:-82
%AX:-126 AY:-382 AZ:-16692 GX:-219 GY:38 GZ:-77
%AX:-102 AY:-388 AZ:-16742 GX:-215 GY:34 GZ:-81
%AX:-84 AY:-370 AZ:-16706 GX:-214 GY:35 GZ:-92
%AX:-76 AY:-398 AZ:-16642 GX:-211 GY:36 GZ:-82
%AX:-98 AY:-424 AZ:-16600 GX:-207 GY:40 GZ:-89
%...
%...
%...

PORT_NAME = "COM14"; % Depends to board
BAUD_RATE = 115200;

s = serialport(PORT_NAME, BAUD_RATE);
s.Timeout = 2;

printf("Serial opened: %s @ %d baud\n", PORT_NAME, BAUD_RATE);
printf("Quit: Ctrl+C...\n");

dt = 0.01;
t_prev = tic;
x = zeros(4,1);
P = eye(4);
% NOTE: F ve B from iteration
% with dt_real
Q = diag([0.001, 0.001, 0.0001, 0.0001]);

% R matrix — from calibration data
% Datas calibrated with
scale_accel = 1.0 / 16384.0;
R_roll  = 702.11  * (scale_accel)^2;
R_pitch = 651.55  * (scale_accel)^2;
R_acc   = diag([R_roll, R_pitch]);
H_acc = [1 0 0 0;
         0 1 0 0];


N_MAX = 5000;
rolls  = zeros(N_MAX, 1);
pitchs = zeros(N_MAX, 1);
i = 0;


figure;
subplot(2,1,1);
h_roll = plot(0, 0, 'b', 'LineWidth', 1.5);
ylabel('Roll (deg)'); grid on; title('Kalman - Roll & Pitch)');
subplot(2,1,2);
h_pitch = plot(0, 0, 'r', 'LineWidth', 1.5);
ylabel('Pitch (deg)'); xlabel('Sample'); grid on;

try
  while i < N_MAX
    line_str = readline(s);
    if isempty(line_str)
      continue;
    end

    vals = sscanf(line_str, 'AX:%d AY:%d AZ:%d GX:%d GY:%d GZ:%d');
    if length(vals) ~= 6
      continue;
    end

    i = i + 1;

    dt_real = toc(t_prev);
    t_prev = tic;
    if dt_real <= 0 || dt_real > 0.5
      dt_real = dt;
    end

  % Later noticed 2 times nias minus
   %{
    F = [1  0  -dt_real  0;
         0  1   0  -dt_real;
         0  0   1   0;
         0  0   0   1];
   %}
    % Now correct
    F = eye(4);
    B = [dt_real  0;
          0  dt_real;
          0   0;
          0   0];

    ax = vals(1) / 16384.0;
    ay = vals(2) / 16384.0;
    az = vals(3) / 16384.0;
    wx = vals(4) / 131.0 * (pi/180.0);
    wy = vals(5) / 131.0 * (pi/180.0);

    % PREDICT
    gyro_corr = [wx; wy] - x(3:4);
    x = F*x + B*gyro_corr;
    P = F*P*F' + Q;

    % UPDATE — accel
    roll_meas  = atan2(ay, sqrt(ax^2 + az^2));
    pitch_meas = atan2(-ax, sqrt(ay^2 + az^2));
    z_acc = [roll_meas; pitch_meas];
    y_inn = z_acc - H_acc*x;
    S     = H_acc*P*H_acc' + R_acc;
    K     = P*H_acc' * inv(S);
    x     = x + K*y_inn;
    P     = (eye(4) - K*H_acc)*P;

    rolls(i)  = rad2deg(x(1));
    pitchs(i) = rad2deg(x(2));

    % every 10 sample
    if mod(i, 10) == 0
      set(h_roll,  'XData', 1:i, 'YData', rolls(1:i));
      set(h_pitch, 'XData', 1:i, 'YData', pitchs(1:i));
      drawnow;
    end

    if mod(i, 100) == 0
      printf("sample=%d roll=%.2f deg pitch=%.2f deg\n", i, rolls(i), pitchs(i));
    end
  end
catch err
  printf("Durduruldu / hata: %s\n", err.message);
end_try_catch

clear s;

N = i;
rolls  = rolls(1:N);
pitchs = pitchs(1:N);

printf("\Samples : %d\n", N);
if N > 0
  printf("Son Roll:  %.4f deg\n", rolls(N));
  printf("Son Pitch: %.4f deg\n", pitchs(N));
  printf("Gyro bias GX: %.6f rad/s\n", x(3));
  printf("Gyro bias GY: %.6f rad/s\n", x(4));
end

% last one
figure;
subplot(2,1,1);
plot(rolls, 'b', 'LineWidth', 1.5);
ylabel('Roll (deg)'); grid on; title('Kalman - Roll & Pitch (final)');
subplot(2,1,2);
plot(pitchs, 'r', 'LineWidth', 1.5);
ylabel('Pitch (deg)'); xlabel('Sample'); grid on;
