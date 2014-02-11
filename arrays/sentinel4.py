from z3 import Reals, Ints, Tactic, Exists, ForAll, Or, And, Implies, Then, Not
i,x,n = Ints('i x n')
# i,x,n = Reals('i x n') h2 does not imply h

#tactic = Then(Tactic('qe'),Tactic('simplify'),Tactic('propagate-values'),Tactic('propagate-ineqs'))
tactic = Tactic('qe')
f = Or(
  And(-i+x==0, -i+n-1==0, i-2>=0),
  And(-n+x+2==0, -i+n-3>=0, i>=0),
  And(x>=0, n-x-3>=0, i-x-1>=0),
  And(-i+x==0, -i+n-2==0, i-1>=0),
  And(-n+x+1==0, -i+n-2>=0, n-3>=0, i>=0),
  And(-i+x-1>=0, n-x-3>=0, i>=0),
  And(-n+x+1==0, n-3>=0, i-n>=0),
  And(-i+x==0, -i+n-3>=0, i>=0))
g = And(n>2, ForAll(x, Implies(And(x>=0, x<n), f)))
h = apply(And,tactic(g)[0])

h2 = And(n>2, i>=0, i<=n-2)
#h2neg = Or(n<=2, i<=-1, i>=n-1)

print tactic(ForAll(x,Implies(h,h2)))
print tactic(ForAll(x,Implies(h2,h)))

#print tactic(ForAll(x,Or(h2neg,h)))
