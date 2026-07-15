clear all
clc
pkg load symbolic
sympref('display', 'flat')

syms phi theta psi real

% Vehicle -> Vehicle-1 (yaw, psi, on k axis)
R_v1_v = [cos(psi), sin(psi), sym(0);
          -sin(psi), cos(psi), sym(0);
          sym(0), sym(0), sym(1)];

% Vehicle-1 -> Vehicle-2 (pitch, theta, on j axis)
R_v2_v1 = [cos(theta), sym(0), -sin(theta);
           sym(0), sym(1), sym(0);
           sin(theta), sym(0), cos(theta)];

% Vehicle-2 -> Body (roll, phi, on i axis)
R_b_v2 = [sym(1), sym(0), sym(0);
          sym(0), cos(phi), sin(phi);
          sym(0), -sin(phi), cos(phi)];

disp('R^v1_v (yaw) =')
disp(R_v1_v)
disp('R^v2_v1 (pitch) =')
disp(R_v2_v1)
disp('R^b_v2 (roll) =')
disp(R_b_v2)



% FULL SEQUENCE: Vehicle -> Body
R_b_v = simplify(R_b_v2 * R_v2_v1 * R_v1_v);

disp('R^b_v (Vehicle -> Body) =')
disp(R_b_v)


% Formula from book
R_book = [cos(theta)*cos(psi), cos(theta)*sin(psi), -sin(theta);
          sin(phi)*sin(theta)*cos(psi)-cos(phi)*sin(psi), sin(phi)*sin(theta)*sin(psi)+cos(phi)*cos(psi), sin(phi)*cos(theta);
          cos(phi)*sin(theta)*cos(psi)+sin(phi)*sin(psi), cos(phi)*sin(theta)*sin(psi)-sin(phi)*cos(psi), cos(phi)*cos(theta)];

disp('Formula from book =')
disp(R_book)

disp('Difference what we enter and waht book give Must be 0 =')
disp(simplify(R_b_v - R_book))


disp('Ortogonal ? (Must be R*R^T = I)')
disp(simplify(R_b_v * transpose(R_b_v)))

disp('det(R_b_v) = ')
disp(simplify(det(R_b_v)))



% Numerical Vehicle
phi_num = sym(10)*sym(pi)/180;  %  X  10 degree
theta_num = sym(15)*sym(pi)/180; % y  15 degree
psi_num = sym(30)*sym(pi)/180;   % z 30 degree

R_b_v_num = double(subs(R_b_v, [phi, theta, psi], [phi_num, theta_num, psi_num]));

disp('R^b_v (numerical, phi=10, theta=15, psi=30 degree) =')
disp(R_b_v_num)

% Vehicle frame "forward" vektor
p_v = [1; 0; 0];

% Step
R_v1_v_num = double(subs(R_v1_v, psi, psi_num));
R_v2_v1_num = double(subs(R_v2_v1, theta, theta_num));
R_b_v2_num = double(subs(R_b_v2, phi, phi_num));

p_v1 = R_v1_v_num * p_v;
printf('inside Vehicle     = [%.4f, %.4f, %.4f]\n', p_v(1), p_v(2), p_v(3));
printf('inside Vehicle-1   = [%.4f, %.4f, %.4f]\n', p_v1(1), p_v1(2), p_v1(3));

p_v2 = R_v2_v1_num * p_v1;
printf('inside Vehicle-2   = [%.4f, %.4f, %.4f]\n', p_v2(1), p_v2(2), p_v2(3));

p_b = R_b_v2_num * p_v2;
printf('inside Body        = [%.4f, %.4f, %.4f]\n', p_b(1), p_b(2), p_b(3));

% MUST BE SAME RESULT
p_b_direct = R_b_v_num * p_v;
printf('inside Body (with R_b_v) = [%.4f, %.4f, %.4f]\n', p_b_direct(1), p_b_direct(2), p_b_direct(3));

disp('Difference (must be zero !!) =')
disp(p_b - p_b_direct)

