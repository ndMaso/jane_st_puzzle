from scipy import optimize as spopt
import numpy as np
import pandas as pd

disallowed_pairs = []
#forbid separated by 11
for i in range(121 - 11):
  disallowed_pairs.append([i, i+11])

for i in range(121):
  if (i-10) >= 0:
    if (i % 11) != 0:
      disallowed_pairs.append([i-10,i])
  if (i % 11) == 0: continue #no forbidding here
  if (i-1)>=0:
    disallowed_pairs.append([i-1, i])
  if (i-12)>=0:
    disallowed_pairs.append([i-12, i])

hyper_edges = []
for i in range(11):
  hyper_edges.append([j + i*11 for j in range(11)]) #need exactly 2 in each low order counter sweep
  hyper_edges.append([i + j*11 for j in range(11)]) #need exactly 2 in each slice of full counter values mod 11
def base11_to_10(a):
  return a%10 +((a-a%10)/10)*11
#custom sets
A=10
group1 = [3*11+9, 3*11+8, 2*11+8, 2*11+7, 1*11+7, 7, 1*11+6]
group2 = [0,1,2,3,4,1*11+0,1*11+1,1*11+3,1*11+4,2*11+0,2*11+1,3*11+0,3*11+1,4*11+1]
group3 = [8*11+3,9*11+3,9*11+4,A*11+3]
group4 = [7*11+1,7*11+2,7*11+3,8*11+1,8*11+2,9*11+2,A*11+1,A*11+2]
group5 = [8,9,1*11+8,1*11+9,2*11+9]
group6 = [A,1*11+A,2*11+A,3*11+7, 3*11+A, 4*11+5,4*11+6,4*11+7,4*11+8,4*11+9,
  4*11+A,5*11+7,6*11+7,7*11+7,8*11+4,8*11+5,8*11+6,8*11+7,9*11+5,9*11+6,
  9*11+7,A*11+4,A*11+5,A*11+6,A*11+7,A*11+8,A*11+9,A*11+A]
group7 = [5*11+8,5*11+9,5*11+A,6*11+8,7*11+8,8*11+8,9*11+8,9*11+9,9*11+A]
group8 = [6*11+9,6*11+A,7*11+9,7*11+A,8*11+9,8*11+A]
group9 = [5,6,1*11+5,2*11+3,2*11+4,2*11+5,2*11+6,3*11+3,4*11+3,5*11+3,6*11+0,6*11+1,
  6*11+2,6*11+3,6*11+4,6*11+5,7*11+0,8*11+0,9*11+0,9*11+1,A*11+0]
groupA = [1*11+2,2*11+2,3*11+2,4*11+0,4*11+2,5*11+0,5*11+1,5*11+2]
group10 = [3*11+4,3*11+5,3*11+6,4*11+4,5*11+4,5*11+5,5*11+6,6*11+6,7*11+4,7*11+5,7*11+6]

agg = [group1,group2,group3,group4,group5,group6,group7,group8,group9,groupA,group10]

agg = pd.Series([i for group in agg for i in group])

for i in range(121):
  if i in agg.values:
    continue
  print([i])
#print(agg.value_counts())


hyper_edges.extend([group1,group2,group3,group4,group5,group6,group7,group8,group9,groupA,group10])

print(hyper_edges[0])
Aeq = np.zeros([len(hyper_edges), 121])
for i in range(len(hyper_edges)):
  Aeq[i, hyper_edges[i]] = 1

beq = np.array([2 for i in range(len(hyper_edges))])

Aub = np.zeros([len(disallowed_pairs), 121])
for i in range(len(disallowed_pairs)):
  Aub[i,disallowed_pairs[i]] = 1

bub = np.array([1 for i in range(len(disallowed_pairs))])

c = np.array([0 for i in range(121)])
#Aub used to specify at most 1 in each pair of two delayed by 11 etc.
#Aeq used to satisfy exactly 2 in each set
bounds = (0,1) #true or false
#spopt.linprog()

res = spopt.linprog(c, A_ub = Aub, b_ub = bub, A_eq = Aeq, b_eq=beq, bounds=bounds, integrality = np.array([1 for i in range(121)]))

print(np.argwhere(np.round(res.x)))