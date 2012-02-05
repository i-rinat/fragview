

"Severity" as fragmentation metric
==================================


Severity is a metric for file fragmentation. Roughly it shows how slow
can be linear read. It equals to fraction of raw disk speed to read
speed of slowest file part. So it estimates effect of gaps between extents
in terms of delays they introduces. Function below uses four parameters:

1. _size of aggregation window (blocks)_ <br/>
   applications that read files continuously (i.e. video players) and
   try to smooth read delays by buffering input stream. This parameter
   is just size of a buffer. While sliding this window through file and
   counting gaps one can estimate effective read speed.
2. _shift (blocks)_ <br/>
   there is such thing as read-ahead. Every movement of hdd heads is very
   slow compared to read of several adjacent sectors, so small enough
   gaps are likely to be read and thrown away, it's faster than skip
   them and cause head jump. But if next file extent located before
   current, head jump become inevitable. So if gap size is between 0 and
   some value, there will be no delay. This parameter is the very value.
3. _penalty (ms)_ <br/>
   hdd random seek delay
4. _speed (blocks/s)_ <br/>
   hdd raw read speed

In ideal case when files continuous, this metric gives 1.0.
The larger the worst. Given value `n` one can say this file is (somewhere
inside) `n` times slower at linear read than continuous one.


