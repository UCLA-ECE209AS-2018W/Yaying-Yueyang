file_list=['data03-20-330-550.txt','data03-21-200-300.txt','data03-21-353.txt','data03-21-357.txt','data03-21-400-530.txt','data03-21-600-700.txt',
           'data03-22-0200-0250.txt','data0322-030-200.txt','data0322-0530-0600.txt']
lu=[]
temp=[]
hum=[]
ultra=[]
for file in file_list:
    input_file=open("../data/" + file)
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

    
        if (command_class=='31'):
            sensor_type=array[11]
            if (sensor_type=='01'):
                temp.append(time)
                temp.append(array)
                continue
            elif (sensor_type=='03'):
                lu.append(time)
                lu.append(array)
                continue
            elif (sensor_type=='05'):
                hum.append(time)
                hum.append(array)
                continue
            elif (sensor_type=='1b'):
                ultra.append(time)
                ultra.append(array)
                continue

f=open("out.txt","w+")
f.write("LU: \n")
for a in lu:
    for el in a:
        f.write(el)
    f.write('\n')
f.write("TEMP: \n")
for a in temp:
    for el in a:
        f.write(el)
    f.write('\n')
f.write("HUM: \n")
for a in hum:
    for el in a:
        f.write(el)
    f.write('\n')
f.write("ULTRA \n")
for a in ultra:
    for el in a:
        f.write(el)
    f.write('\n')
            
