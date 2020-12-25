#__DEBUG__=1
#__DEBUG_LEX__=1

echo -n "Test1 "
false && true && echo "Failed"
true && true && echo "Ok"

echo -n "Test2 "
false || false || echo "Ok"
true && false && echo "Failed"

echo -n "Test3 "
! true && echo "Failed"
! true || echo "Ok"
