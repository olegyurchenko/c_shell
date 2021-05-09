#__DEBUG__ = 1

if true ; then echo "Test1 Ok" ; fi ; echo 1111 |
if false
then echo "Test2 failed"
else echo "Test2 Ok"
fi

if true ; then 
  echo "Test3 Ok" 
fi

if false ; then 
  echo "Test4 failed"
else 
  echo "Test4 Ok"
fi

if false ; then
  if false ; then
    echo "Test5 failed"
  else
    echo "Test6 failed"
  fi
  if true ; then
    echo "Test5 failed"
  else
    echo "Test6 failed"
  fi
  echo "Test7 failed"
else
  echo "Test5 Ok"
fi

if true ; then
  if false ; then
    echo "Test8 failed"
  else
    echo "Test6 Ok"
  fi
  echo "Test7 Ok"
else
  echo "Test9 Failed"
fi
