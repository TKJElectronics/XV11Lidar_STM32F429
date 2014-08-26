global fid;

fid = serial('COM74','BaudRate',115200);
set(fid,'InputBufferSize',400000);  %  Set serial buffer size >(6+360x4)*2=2892 byte
set(fid,'Timeout',1);             %  Set read Timeout in 1 sec
fopen(fid);

%% Setup plot for lidar
figure(1);
lidarplot = plot(zeros(1,360),zeros(1,360),'.'); % plot something
axis equal;
axis([-2000 2000 -2000 2000]);
grid on;
xlabel('X mm'),ylabel('Y mm');
drawnow;
pause(.1);

if (fid.BytesAvailable > 0)
    fread(fid, fid.BytesAvailable, 'uint8'); % empty serial buffer
end

Distance = zeros(1,360,'uint16');
MessageBox = msgbox( 'Displaying LIDAR', 'XV-11 LIDAR' );

while ishandle( MessageBox )
    while (fid.BytesAvailable < (2*360 + 6) && ishandle( MessageBox ))
      pause(.01); % wait 10ms for data come in
    end
    
    if (~ishandle( MessageBox ))
        break;
    end
    
    read = fread(fid, 3, 'uint8');
    if (read(1) == hex2dec('AA') && read(2) == hex2dec('BB') && read(3) == hex2dec('CC'))
        disp('OK');
        for Index = 1:360
            Distance(Index) = fread(fid, 1, 'uint16' );
        end
        
        read = fread(fid, 3, 'uint8');
        if (read(1) == hex2dec('CC') && read(2) == hex2dec('BB') && read(3) == hex2dec('AA'))
            disp('END OK');
            
           [X,Y] = pol2cart(0:2*pi/360:2*pi*(1-1/360), double(Distance));
           %   plot(X,Y,'.'),axis equal,axis([-500 500 -500 500]),
           set(lidarplot,'xdata',X,'ydata',Y);
           drawnow            
        end
    end
    if (fid.BytesAvailable > 0)
        fread(fid, fid.BytesAvailable, 'uint8'); % empty serial buffer
    end
end

fclose(fid);