os.chdir(r'C:\Users\esultano\git\number_fields\python\notebooks')
import csv

load('input.sage')
f=open('output.csv','w')
out = csv.writer(f, delimiter=',',quoting=csv.QUOTE_ALL)
for row in data:
    out.writerow(row)
f.close()
