# Assembly
It isn't that bad.

## Register assignment
Don't assign registers until you're done with the program. Just write the code using the register name `todo`, and when you're done assign registers according to the following rules:
- Variables prefixed with `r` correspond to general registers `a[0-15]` and should be assigned in descending order.
- Variables prefixed with `v` correspond to vector registers `q[0-7]` and should be assigned in ascending order.

Be careful to keep track of when each variable is `#define`'d and `#undef`'d to use as few registers as possible!

## Style
- Prefix general-purpose registers `a[7-11]` with `r_`
- Prefix vector registers `q[0-7]` with `v_`
- Prefix static labels with `.l_`
- Prefix symbols representing an address with `p_`
- Naming registers containing constants:
    - Mangle `-` as `neg`
    - Mangle `.` as `_`
    - Prefix hex numbers with `h`
    - Prefix binary numbers with `b`
    - Prefix floating point numbers with `f`
