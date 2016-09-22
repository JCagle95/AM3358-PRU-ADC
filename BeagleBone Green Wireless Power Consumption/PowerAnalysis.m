%% Boot up Power

Boot = load('BBGW_Boot.mat');
Time = Boot.storedData(2,:) / 1000; % in seconds
Current = Boot.storedData(1,:) / 1000; % in Amps;

% Zero-delay Filtering
smoothCurrent = Current;
Weight = Current(1);
for i = 1:length(Current)
    smoothCurrent(i) = Weight;
    Weight = Weight + 0.05 * (Current(i)-smoothCurrent(i));
end

% Compute Transfer Function
PxxOriginal = periodogram(Current,hanning(length(Current)),0:0.25:42,85);
PxxFiltered = periodogram(smoothCurrent,hanning(length(Current)),0:0.25:42,85);
TransferFunc = PxxFiltered ./ PxxOriginal;

% Select Boot Period (0 to 100 sec);
smoothCurrent = smoothCurrent(Time <= 101);
Current = Current(Time <= 101);
Time = Time(Time <= 101);

% Plotting
h = largeFigure(1000,[3440 1380]); clf; hold on;
% Plot 1: Time Trend of Power Consumption
subplot(1,2,1); cla; hold on;
plot(Time,Current*5,'b','linewidth',1);
plot(Time,smoothCurrent*5,'r','linewidth',2);
plot([20.55,20.55],[0 3],'k--','linewidth',3);
plot([58.22,58.22],[0 3],'k--','linewidth',3);
axis([0 Time(end) 0 2.5])
xlabel('Time (sec)','fontsize',15);
ylabel('Power (Watts)','fontsize',15);
text(mean([0,20.55]),0.45*5,'Off','fontsize',18,'HorizontalAlignment','center');
text(mean([20.55,58.22]),0.45*5,'Bootup','fontsize',18,'HorizontalAlignment','center');
text(mean([Time(end),58.22]),0.45*5,'Idle','fontsize',18,'HorizontalAlignment','center');
% Plot 2: Cummulative Power Consumption in mWh
subplot(1,2,2); cla; hold on;
BootingPower = cumsum(5*Current*1000.*[diff(Time),mean(diff(Time))]/60/60);
plot(Time,BootingPower,'m','linewidth',2);
plot([20.55,20.55],[0 30],'k--','linewidth',3);
plot([58.22,58.22],[0 30],'k--','linewidth',3);
axis([0 Time(end) 0 30])
xlabel('Time (sec)','fontsize',15);
ylabel('Cumulative Power Consumption (mWh)','fontsize',15);
text(mean([0,20.55]),0.45*60,'Off','fontsize',18,'HorizontalAlignment','center');
text(mean([20.55,58.22]),0.45*60,'Bootup','fontsize',18,'HorizontalAlignment','center');
text(mean([Time(end),58.22]),0.45*60,'Idle','fontsize',18,'HorizontalAlignment','center');
suptitle('BeagleBone Green Wireless Bootup Power Consumption');
print(h,'Bootup Power','-dpng','-r500');

%% PRU Execution and Data Storage

Boot = load('PRU_SlowerWriting.mat');
Time = Boot.storedData(2,201:end) / 1000; % in seconds
Current = Boot.storedData(1,201:end) / 1000; % in Amps;

% Zero-delay Filtering
smoothCurrent = Current;
Weight = Current(1);
for i = 1:length(Current)
    smoothCurrent(i) = Weight;
    Weight = Weight + 0.05 * (Current(i)-smoothCurrent(i));
end

% Select Boot Period (0 to 80 sec);
smoothCurrent = smoothCurrent(Time <= 81);
Current = Current(Time <= 81);
Time = Time(Time <= 81);

% Compute Relative Frequency
binPos = 0.8:0.01:1.8;
Count = hist(smoothCurrent(Time <= 33)*5,binPos);
RelFreq_Idle = Count/sum(Time<=33)/0.01;
Count = hist(smoothCurrent(Time > 33)*5,binPos);
RelFreq_PRU = Count/sum(Time<=33)/0.01;
[~,index(1)] = max(RelFreq_Idle);
[~,index(2)] = max(RelFreq_PRU);

% Plotting
h = largeFigure(1001,[3440 1380]); clf; hold on;
% Plot 1: Time Trend of Power Consumption
subplot(1,2,1); cla; hold on;
plot(Time,Current*5,'b','linewidth',1);
plot(Time,smoothCurrent*5,'r','linewidth',2);
plot([33,33],[0 3],'k--','linewidth',3);
axis([Time(1) Time(end) 0.8 2.5])
xlabel('Time (sec)','fontsize',15);
ylabel('Power (Watts)','fontsize',15);
text(mean([Time(1),33]),0.45*5,'Idle','fontsize',18,'HorizontalAlignment','center');
text(mean([Time(end),33]),0.45*5,'PRU Execution','fontsize',18,'HorizontalAlignment','center');
% Plot 2: Distribution Comparison of PRU and Idle
subplot(1,2,2); cla; hold on;
plot(binPos,RelFreq_Idle,'b','linewidth',2);
plot(binPos,RelFreq_PRU,'r','linewidth',2);
plot(binPos(index),1.05*max([RelFreq_Idle(index(1)),RelFreq_PRU(index(2))])*ones(1,2),'k','linewidth',4);
axis([binPos(1) binPos(end) 0 33])
text(mean(binPos(index)),1.1*max([RelFreq_Idle(index(1)),RelFreq_PRU(index(2))]), ...
    sprintf('Difference in Mode: %.3f Watts',diff(binPos(index))),'fontsize',10,'HorizontalAlignment','center');
xlabel('Power (Watts)','fontsize',15);
ylabel('Relative Frequency (%)','fontsize',15);
Legend_Font(gca,{'Idle','PRU Execution'},12);
suptitle('BeagleBone Green Wireless PRU Power Consumption');
print(h,'PRU Power','-dpng','-r500');

