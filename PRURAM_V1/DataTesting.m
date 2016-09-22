
file = dir('Data*.bin');
for index = 0:length(file)-1
    fid = fopen(sprintf('Data%d.bin',index),'rb');
    Data = fread(fid, 1000, 'uint32');
    fclose(fid);
    
    % Error Checking
    if (sum(diff(Data)~=4) > 1)
        fprintf('The Data Difference is more than 4. Error in PRU Writing in %s\n',sprintf('Data%d.bin',index));
    end
    
    if (Data(1)~=4+4000*(index))
        fprintf('Inaccurate Data in %s\n',sprintf('Data%d.bin',index));
    end
   
end
