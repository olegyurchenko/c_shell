echo -n 'test ' | read xxx >f1 2>/dev/null
echo Ok >> f1
read < f1
