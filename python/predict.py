file_list=['data03-20-330-550.txt']
freq={}
home_map={}
command_map={'71':'Alarm','9c':'Alarm Sensor', '9d': 'Alarm Silence', '27':'All Switch', '5d':'Anti-theft','57':'Application Campability', '22': 'Application Status', '85':'Association', '9b':'Association Command Configuration','59': 'Association Group Information', '66':'Barrier Operator', '20':'Basic' ,'36':'Basic Tariff information','50':'Basic Window Covering','80':'Battery', '30': 'Binary Sensor','25': 'Binary Switch','28':'Binary Switch','46': 'Binary Toggle Switch','5b': 'Climate Control Schedule', '81': 'Clock', '33':'Clor Switch','70':'Configuration', '21': 'Controller Replication',
            '56':'CRC-16 Encapsulation','3a':'Demand Control Plan Configuration','3b':'Demand control Plan Monitor','5a':'Device Reset Locally', '62':'Door Lock','4c':'Door Lock Logging',
            '90':'Energy Production','6f':'Entry Control','7a':'Firmware Update Meta Data','8c':'Geographic Location','7b':'Grouping Name', '82':'Hail','37':'HRV Status', 
            '6d': 'Humidity Control Mode','6e':'Humidity Control Operating State', '64':'Humidity Control Setpoint','74':'Inclusion Controller','87':'Indicator','5c': 'Ip Association',
            '9a':'IP Configuration', '6b':'Irrigation', '89':'Language', '76':'Lock','69':'Mailbox','91':'Manufacturer proproetary','72':'Manufacturer Specific','ef':'Mark (Suppoer/Control Mark)',
            '32':'Meter','3c': 'Meter Table Configuration','3d':'Meter Table Monitor','3e': 'Meter Table Push Configuration','51':'Move To Position Window Covering','60':'Multi Channel',
            '8e':'Multi Channel Association','8f':'Multi Command','31':'Multilevel Sensor','26':'Multilevel Switch','29':'Multilevel Toggle Switch', '4d':'Network Management Basic Node','34':
            'Network Management Inclusion', '67':'Network Management Install and Maintenace','54':'Network Management Primary','52':'Network Management Proxy', '00':'No Operation','77':"Node Naming and Location",
            '78':'Node Provisioning','71':'Notification','73':'Powerlevel','3f':'Prepayment','41':'Prepayment Encapsulation','88':'Proproetary','75':'Protection','35':'Pulse Meter',
            '48':'Rate Table Configuration','49':'Rate Table Monitor','7c':'Remote Association Activation','7d':'Remote Association Configuration', '2b':'Scene Acitvation', '2c':'Scene Actuator Configuration',
            '2d':'Scene Controller Configuration','53':'Schedule','4e': 'Schedule Entry Lock','93':'Screen Attributes','92':'Screen Meta Data','98':'Security 0', '9f':'Security 2','9e':'Sensor Confuguration',
            '94':'Simple AV Control','79':'Sound Switch','6c':'Supervision','4a':'Tariff Table Configuration','4b':'Tariff Table Monitor','44':'Thermostat Fan Mode','45':'Thermostat Fan State',
            '40':'Thermostat','42':'Thermostat Operating State','47':'Thermostat Setback','43':'Thermostate Setpoint','8a':'Time','8b':'Time Parameters','55':'Transport','63':'User Code',
            '86':'Version','84':'Wake Up','6a':'Window Covering','23':'Z/IP', '4f':'Z/IP 6LoWPAN','5f':'Z/IP Gateway','68':'Z/IP Naming and Location','58':'Z/IP ND', '61':'Z/IP Portal',
            '5e': 'Z-Wave Plus Info'}
for file in file_list:
    input_file=open(file)
    for line in input_file:
        tokens=line.split('@')
        if(len(tokens)<2):
            continue
        time=tokens[0]
        datas=tokens[1]
        array=datas.split(' ')
        if(len(array)<11):
            continue
        home_id=array[0]+array[1]+array[2]+array[3]
        if home_id in freq:
            freq[home_id]+=1
        else:
            freq[home_id]=0

    for key in freq:
        if freq[key]>10:
            home_map[key]={}
    input_file=open(file)
    for line in input_file:
        tokens=line.split('@')
        if(len(tokens)<2):
            continue
        time=tokens[0]
        datas=tokens[1]
        array=datas.split(' ')
        if(len(array)<11):
            continue
        home_id=array[0]+array[1]+array[2]+array[3]
        if home_id in home_map:
            source_node=array[4]
            command_class=array[9]
            if source_node in home_map[home_id]:
                if command_class in home_map[home_id][source_node]:
                    continue
                else:
                    if command_class in command_map:
                        home_map[home_id][source_node][command_class]=command_map[command_class]
            else :
                if command_class in command_map:
                    home_map[home_id][source_node]={}
                    home_map[home_id][source_node][command_class]=command_map[command_class]

    for a in home_map:
        print("Found home device: ",a)
        print("It has following node: ")
        for b in home_map[a]:
            print("Node ",b)
            print("This Node has the following command class: ")
            for c in home_map[a][b]:
                print (home_map[a][b][c])

    
    
            
        
