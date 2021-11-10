"""
Python script to compute hashes
"""
def hash(str):
  h = 5381
  for c in str:
    h = (h * 33 + ord(c)) & 0xffffffff
  return h

names = ("exit"
, "break"
, "continue"
, "if"
, "then"
, "fi"
, "else"
, "elif"
, "true"
, "false"
, "while"
, "until"
, "for"
, "do"
, "done"
, "echo"
, "test"
, "["
, "[["
, "set"
, "read"
, "tee"
, "seq"
, "let"
)

comma = " "
print()
for name in names:
  print( '{0} {{ 0x{1:08x}, "{2}", _{3} }}'.format(comma, hash(name), name, name))
  comma=','
