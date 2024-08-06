import csv
import sys

filename = sys.argv[1]
device_id = sys.argv[2]
print("Parsing csv ", filename)
with open(filename, newline='') as csvfile:
    rows = csv.DictReader(csvfile)
    cnt = [0 for i in range(8)] 
    preF = [0.0 for i in range(8)] 
    postF = [0.0 for i in range(8)] 

    for row in rows:
        for i in range(8):
            rowName = "GPU"+device_id+" XCD XCD" + str(i) + " Busy"
            targetFreq = "GPU"+device_id+" XCD XCD" + str(i) + " Target Freq"
            preDeepFreq = "GPU"+device_id+" XCD XCD" + str(i) + " Pre Deep Sleep Freq"
            postDeepFreq = "GPU"+device_id+" XCD XCD" + str(i) + " Post Deep Sleep Freq"
            if row[rowName]:
                bz = float(row[rowName])
                if bz >= 99.0:
                    cnt[i]+=1
                    # print(rowName)
                    # print(row['Time Stamp'])
                    # print(bz)
                    # print(row[targetFreq])
                    # print(row[preDeepFreq])
                    # print(row[postDeepFreq])
                    preF[i]+=float(row[preDeepFreq])
                    postF[i]+=float(row[postDeepFreq])
    for i in range(8):
        if cnt[i]>0:
            postF[i] = postF[i]/cnt[i]
            #print("=======XCD",i,", frequency: ", postF[i], ", count = ",cnt[i])
            # print("Avg pre freq = ",preF[i])
            # print("Avg post freq = ",postF[i])
    #print("======min freq : ", min(postF))
    print("======avg freq : ", sum(postF)/8)

