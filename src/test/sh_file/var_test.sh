echo $0
#__DEBUG__ = 1

x="Test var"

if [ "$x" == "Test var" ] ; then
  echo "Var test 1 Ok"
else
  echo "Var test 1 Failed"
fi

var123="1234"
z="${var123} 1234"

if [ "$z" == "1234 1234" ] ; then
  echo "Var test 2 Ok"
else
  echo "Var test 2 Failed"
fi

#exit 1


var=1234
if [ $var == 1234 ] ; then
  echo "Var test 3 Ok"
else
  echo "Var test 3 Failed"
fi

var="   1"
if [ $var ] ; then
  echo "Var test 4 Ok"
else
  echo "Var test 4 Failed"
fi

var="   "
if [ $var ] ; then
  echo "Var test 5 Failed"
else
  echo "Var test 5 Ok"
fi
