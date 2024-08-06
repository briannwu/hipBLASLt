import csv
import sys

filename = sys.argv[1]
device_id = sys.argv[2]
#print("Parsing csv ", filename)
with open(filename, newline='') as csvfile:
    rows = csv.DictReader(csvfile)
    fclk = [0.0 for i in range(4)]
    uclk = [0.0 for i in range(4)]
    clk_cnt = 0
    for row in rows:
        active = True
        for i in range(8):
            rowName = "GPU"+device_id+" XCD XCD" + str(i) + " Busy"
            if row[rowName]:
                bz = float(row[rowName])
                if bz < 99.0:
                    active = False
        if active==True:
            clk_cnt += 1
            for aid in range(4):
                fclk_eff_Freq = "GPU"+device_id+" Effective Frequencies FCLK Eff AID" + str(aid)
                uclk_a_eff_Freq = "GPU"+device_id+" Effective Frequencies UCLK_a Eff AID" + str(aid)
                fclk[aid] += float(row[fclk_eff_Freq])
                uclk[aid] += float(row[uclk_a_eff_Freq])

    for i in range(4):
        if clk_cnt > 0:
            fclk[i] = fclk[i]/clk_cnt
            uclk[i] = uclk[i]/clk_cnt
    print("========avg fclk freq : ", sum(fclk)/4)
    print("========avg uclk freq : ", sum(uclk)/4)

