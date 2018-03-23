import matplotlib.dates as mlab
import matplotlib.pyplot as plt
from datetime import datetime

file_list=['../data/data03-20-330-550.txt']
for file in file_list:
    input_file = open(file)
    times=[]
    actions=[]
    for line in input_file:
        tokens=line.split('@')
        if(len(tokens)<2):
            continue
        time=tokens[0]
        datas=tokens[1]
        array=datas.split(' ')
        if(len(array)<11) :
            continue
        home_id=array[0]+array[1]+array[2]+array[3]
        
        if(home_id != 'c3f673a5'):
            continue
        command_class=array[9]

    
        if (command_class!='30'):
            continue
        else:
            print (array[11])
            if (array[11]=='00'):
                actions.append(0)
                datatime_ob=datetime.strptime(time, '%Y-%m-%d.%X')
                print (datatime_ob)
                times.append(datatime_ob)
            elif (array[11]=="ff"):
                actions.append(1)
                datatime_ob=datetime.strptime(time, '%Y-%m-%d.%X')
                times.append(datatime_ob)

        
dates = mlab.date2num(times)
print(dates)
plt.plot_date(dates, actions,linestyle='solid')
plt.show()

    
    
