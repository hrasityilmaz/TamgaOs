% Kalman live plot from STM32 UART
% Format: "R:roll P:pitch\r\n"

clear all; clc; close all;
pkg load instrument-control

PORT  = "COM14";
BAUD  = 115200;
N_MAX = 3000;

s = serialport(PORT, BAUD);
s.Timeout = 2;

printf("Connected. Ctrl+C to stop.\n");

rolls  = zeros(N_MAX, 1);
pitchs = zeros(N_MAX, 1);
n = 0;

figure;
subplot(2,1,1);
h_r = plot(0, 0, 'b', 'LineWidth', 1.5);
ylabel('Roll (deg)'); grid on;
title('TamgaOS Kalman — Roll & Pitch');
ylim([-90 90]);

subplot(2,1,2);
h_p = plot(0, 0, 'r', 'LineWidth', 1.5);
ylabel('Pitch (deg)'); xlabel('Sample (10ms)'); grid on;
ylim([-90 90]);

try
  while n < N_MAX
    line_str = readline(s);
    if isempty(line_str); continue; end

    vals = sscanf(line_str, 'R:%f P:%f');
    if length(vals) ~= 2; continue; end

    n = n + 1;
    rolls(n)  = vals(1);
    pitchs(n) = vals(2);

    if mod(n, 10) == 0
      set(h_r, 'XData', 1:n, 'YData', rolls(1:n));
      set(h_p, 'XData', 1:n, 'YData', pitchs(1:n));
      drawnow;
    end

    if mod(n, 100) == 0
      printf("n=%d  Roll=%.2f  Pitch=%.2f\n", n, rolls(n), pitchs(n));
    end
  end
catch e
  printf("Stopped: %s\n", e.message);
end_try_catch

clear s;
printf("Total: %d samples\n", n);
