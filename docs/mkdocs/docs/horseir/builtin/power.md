# A Collection of power Functions

## power

*Note*:

```no-highlight
sqrt(x) == power(x, 0.5)
exp(x)  == power(e, x)
```

### Description

Power with base and exponent

- `@power(base, exponent)`

### Type Rules

High-level (without complex)

```no-highlight
Real ,Real -> MaxType -> bool -> f32
                         i8   -> f32
                         i16  -> f32
                         i32  -> f32
                         i64  -> f64
                         f32  -> f32
                         f64  -> f64
_    ,_    -> domain error
```

!!! tip "Note"
    The base and exponent can be either a real number or a complex number.

### Examples

```no-highlight
    @power((2,3,4):i32, 2:i32)
> (4,9,16):f64
```

## sqrt

### Description

Square root

- `@sqrt(value)`


### Type Rules

```no-highlight
bool -> f32
char -> f32 / complex
i16  -> f32 / complex
i32  -> f32 / complex
i64  -> f64 / complex
f32  -> f32 / complex
f64  -> f64 / complex
```


### Example

```no-highlight
    @sqrt((4,9,16):i32)
(2,3,4):f32
```


## exp

### Description

Exponential

- `@exp(value)`
- base is Euler's number, e


### Type Rules

```no-highlight
bool -> f32
char -> f32 / complex
i16  -> f32 / complex
i32  -> f32 / complex
i64  -> f64 / complex
f32  -> f32 / complex
f64  -> f64 / complex
```


### Example

```no-highlight
    @exp(1:i32)
2.7183:f32
```

