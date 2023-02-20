import os
import sys

if 'add_dll_directory' in dir(os):
    for path_directory in sys.path:
        try:
            print('Adding a path to dll directory search path:', path_directory)
            os.add_dll_directory(path_directory)
        except Exception:
            print('I guess not.')

import adftool

f = adftool.File ()

try:
    os.remove('test.adf')
except Exception:
    pass

f.open ('test.adf', True)

integer_object = adftool.Term ()
double_object = adftool.Term ()
double_integer_object = adftool.Term ()
date_object = adftool.Term ()
foo_object = adftool.Term ()
hello_object = adftool.Term ()
integer_object.set_integer (42)
double_object.set_double (49.3)
double_integer_object.set_double (69)
date_object.set_date ((0, 12345678))
foo_object.set_literal ('foo', None, None)
hello_object.set_literal ('hello, world!', None, 'en-us')

subject = adftool.Term ()
predicate = adftool.Term ()
subject.set_named ('')
predicate.set_named ('https://example.com/test')

statement = adftool.Statement ()
statement.set_subject (subject)
statement.set_predicate (predicate)

for o in [ integer_object, double_object, double_integer_object, date_object, foo_object, hello_object ]:
    statement.set_object (o)
    f.insert (statement)

blank = adftool.Statement ()
everything = [ adftool.Statement () for _ in range(100) ]
n_everything = f.lookup (blank, 0, everything)
everything = everything[0:n_everything]
print('Everything:')
for st in everything:
    st_s, st_p, st_o, st_g, st_d = st.get ()
    n3_buffer = bytearray(1024)
    as_str = ''
    for term in [ st_s, st_p, st_o ]:
        as_str = as_str + n3_buffer[0:term.to_n3 (0, n3_buffer)].decode('utf-8') + ' '
    as_str = as_str + '.'
    print(as_str)

integers = [ 0, 0 ]
doubles = [ 0.0, 0.0, 0.0 ]
dates = [ (0, 0) ]
strings = [ b'', b'' ]

n_integers = f.lookup_integer (subject, 'https://example.com/test', 0, integers)
n_doubles = f.lookup_double (subject, 'https://example.com/test', 0, doubles)
n_dates = f.lookup_date (subject, 'https://example.com/test', 0, dates)
n_strings = f.lookup_string (subject, 'https://example.com/test', 0, strings)

print('Integers:', integers)
print('Doubles:', doubles)
print('Dates:', dates)
print('Strings:', strings)

assert n_integers == 2
assert n_doubles == 3
assert n_dates == 1
assert n_strings == 2

assert integers[0] == 42
assert integers[1] == 69
assert abs(doubles[0] - 49.3) < 1e-6
assert doubles[1] == 42
assert doubles[2] == 69
assert dates[0][0] == 0
assert dates[0][1] == 12345678
assert strings[0][0] == b'foo'
assert strings[0][1] is None
assert strings[1][0] == b'hello, world!'
assert strings[1][1] == b'en-us'
