from z3 import Bools, Ints, Exists, ForAll, Or, And, Implies, Not
from z3 import Tactic, Then, OrElse, Repeat, ParThen

n,p,q,x,y = Ints('n p q x y');

middle = Or(
  And(-q+y+1==0, -q+x+1==0, -p+q-2>=0, p>=0, n-q-1>=0),
  And(-q+x+1==0, -p+y-1>=0, q-y-2>=0, p>=0, n-q>=0),
  And(-n+y+1==0, -n+x+1==0, -n+q==0, p>=0, n-p-2>=0),
  And(-q+y+1==0, -q+x+1==0, p+1==0, q-1>=0, n-q-1>=0),
  And(-q+x+1==0, p+1==0, y>=0, q-y-3>=0, n-q-4>=0),
  And(-n+y+1==0, -n+x+1==0, -n+q==0, p+1==0, n-1>=0),
  And(-p+x==0, -q+y+1==0, -p+q-2>=0, p>=0, n-p-3>=0, n-q>=0),
  And(-p+x==0, -p+y-1>=0, q-y-1>=0, p>=0, n-q>=0, n-y-2>=0),
  And(-n+y+1==0, -n+q==0, -p+x==0, p>=0, n-p-2>=0),
  And(-n+q==0, -p+x==0, -p+y-1>=0, p>=0, n-y-2>=0),
  And(-q+y+1==0, -p+q-2>=0, p-x-1>=0, p+1>=0, n-q>=0, n+q-x-6>=0, n+q-4>=0),
  And(-p+y-1>=0, q-y-1>=0, p-x-1>=0, p+1>=0, n-q>=0, n-y-2>=0),
  And(-n+y+1==0, -n+q==0, x>=0, p-x-1>=0, n-p-2>=0),
  And(-n+q==0, -p+y-1>=0, x>=0, p-x-1>=0, n-y-2>=0),
  And(-q+y+1==0, -p+x-1>=0, q-x-2>=0, p>=0, n-q>=0),
  And(-p+y-1>=0, -p+x-1>=0, q-x-1>=0, q-y-1>=0, p>=0, n-q>=0, n-x-2>=0, n-y-2>=0, 3*n-q-x-y-5>=0, 4*n-p-2*q-8>=0),
  And(-n+y+1==0, -n+q==0, -p+x-1>=0, p>=0, n-x-2>=0),
  And(-n+q==0, -p+x-1>=0, -x+y>=0, p>=0, n-y-2>=0),
  And(-q+y+1==0, p+1==0, q-x-2>=0, q-1>=0, n-q-1>=0, n-3>=0),
  And(p+1==0, y>=0, q-x-1>=0, q-y-1>=0, n-q-1>=0),
  And(-n+y+1==0, -n+q==0, p+1==0, x>=0, n-x-2>=0),
  And(-n+q==0, p+1==0, -x+y>=0, x>=0, n-y-2>=0),
  And(-q+y+1==0, -q+x==0, -p+q-2>=0, q-3>=0, p>=0, n-q>=0),
  And(-q+x==0, -p+y-1>=0, q-y-2>=0, p>=0, n-q>=0),
  And(-q+y+1==0, -p+q-2>=0, -q+x-1>=0, x-4>=0, p>=0, n-q>=0, n-3>=0),
  And(-p+y-1>=0, -q+x-1>=0, q-y-1>=0, p>=0, n-q>=0, n-y-2>=0),
  And(-q+x==0, p+1==0, y>=0, q-y-2>=0, n-q-5>=0),
  And(p+1==0, -q+x-1>=0, y>=0, q-y-1>=0, n+q-2*x-4>=0) );

middle2 = ForAll(x, And(y>=0, y<n, Implies(And(0<=x, x<=y), middle)));

qe = Tactic('qe')
simplify=Repeat(Then('nnf','ctx-solver-simplify'))
def simpl(f):
  return apply(And, Then(qe, simplify)(f)[0])

middle3 = simpl(middle2);

# WOO HOO!
# And(Or(p == -1, p >= 0),
#   -1*p + y >= 1,
#    q + -1*y >= 1,
#    n + -1*q >= 0)

