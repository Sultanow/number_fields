os.chdir(r'C:\Users\esultano\git\number_fields\python\notebooks')
import csv

load('class number 1_ Disc 100000001-1000000000(end).sage')
f=open('class number 1_ Disc 10000001-100000000.csv','w')
out = csv.writer(f, delimiter=',',quoting=csv.QUOTE_ALL)
for row in data:
    out.writerow(row)
f.close()
