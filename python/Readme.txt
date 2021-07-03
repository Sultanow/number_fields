os.chdir(r'C:\Users\esultano\Downloads')
load('fields.sage')

import csv
out = csv.writer(open("fields.csv","w"), delimiter=',',quoting=csv.QUOTE_ALL)
for row in data:
    #print(row)
    out.writerow(row)
