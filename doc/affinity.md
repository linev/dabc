# Affinity in dabc {#dabc_affinity}

This document describes how affinity of whole process or single thread can be assigned

## Using taskset to start dabc

Already before it was possible to specify taskset arguments for complete dabc process in
`<Run>` section in the form like `<taskset value="-c 10-15"/>`.
This is used in dabc_run script.


## Assign special arguments for the main process

One can specify default affinity for every new created thread, providing "affinity"
parameter in `<Run>` section. Value can be following:
- unsigned value with processors mask
- string like "xxxoooxxxss" with allowed symbols 'x', 'o' and 's'.
   'x' - enabled, 'o' - disabled, 's' - special purpose
   first element in string corresponds to first processor
- string like "-N" where N is processors number which should be reserved
   for special purposes. These processors could be later assigned
    with SetAffinity("+M") call (M<N)


## Individual settings for the thread

For every thread attribute "affinity" could be specified. Allowed values:
- unsigned value with processors mask
- string like "xxxoooxxx" were x and o identified enabled and disabled processors,
   first element in string corresponds to first processor
- string like "+M" where M is processor number in special processors set,
    before SetDfltAffinity("-N") should be called (M<N)
