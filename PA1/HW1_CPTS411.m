send = csvread('results_send.csv');
rec = csvread('results_rec.csv');

send_unblock = csvread('/Users/paul/Desktop/411HW/results_send_unblock.csv');
rec_unblock = csvread('results_recv_unblock.csv');

figure(1);
rows = send(:,2);
cols = send(:,1);
plot(cols(17:27), rows(17:27));

hold on
rows_r = rec(:,2);
cols_r = rec(:,1);
plot(cols_r(17:27), rows_r(17:27));

legend('send', 'receive');
xlabel('message size m');
ylabel('time in microseconds for block');

figure(2)
rows_ub = send_unblock(:,2);
cols_ub = send_unblock(:,1);
plot(cols_ub(17:27), rows_ub(17:27));

hold on
rows_r_ub = rec_unblock(:,2);
cols_r_ub = rec_unblock(:,1);
plot(cols_r_ub(17:27), rows_r_ub(17:27));

legend('send', 'receive');
xlabel('message size m');
ylabel('time in microseconds for unblock');

%All ones are for blocking, all twos are for unblocking
