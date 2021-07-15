import csv
from itertools import zip_longest

var('x')

all_poly = [x**2 - x + 1,x**2 + 1,x**2 - x - 1,x**2 - x + 2,x**2 + 2]
list_signature=[]
list_discriminant=[]
list_RamifiedPrimes=[]
list_UnitRank=[]
list_ClassGroup=[]
list_regulator=[]
list_CoeffZF=[]

for i in all_poly : 
    K.<x> = NumberField(i)
    
    # Signature: 
    list_signature.append(K.signature())

    # Discriminant: 
    list_discriminant.append(K.disc())
    
    # Ramified primes: 
    list_RamifiedPrimes.append(K.disc().support())
    
    # Unit group: 
    UK = K.unit_group()

    # Unit rank: 
    list_UnitRank.append(UK.rank())

    # Regulator: 
    list_regulator.append(K.regulator())

    #Coefficients
    list_CoeffZF.append(K.zeta_coefficients(1000))

data = [all_poly, list_signature, list_discriminant, list_RamifiedPrimes, list_UnitRank, list_regulator, list_CoeffZF]
rows = zip_longest(*data, fillvalue = '')
f=open('output.csv','w')
out = csv.writer(f, delimiter=',', quoting=csv.QUOTE_ALL)
out.writerow(('poly', 'signature', 'discriminant', 'RamifiedPrimes', 'UnitRank', 'regulator', 'CoeffZF'))
out.writerows(rows)
f.close()