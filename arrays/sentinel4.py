from z3 import Ints, Tactic, Exists, ForAll, Or, And, Implies, Then
i,x,n = Ints('i x n')

#tactic = Then(Tactic('qe'),Tactic('simplify'))
tactic = Tactic('qe-light')
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
print tactic(g)


