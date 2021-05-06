echo -n 'test ' | rr xxx >f1 2>/dev/null
echo Ok >> f1
rr < f1
