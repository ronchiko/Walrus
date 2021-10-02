# Walrus (.Wal) - Data File:

## Properties
Properties are the base of everthing, and they are a key-value pairs, defined like so
`<property> = <value>`


## Values
Values are defined as the things that can be assinged to properties in one line (not object/lists), and they are:
* Integer (primitive): A 32 bit integer number (like 3, -50, 10000)
* Decimal (primitive): A 32 bit floating point (like 3.0, -.3, 10.3e-10), sometimes known as floats or vectors
* Boolean: Either 'true' or 'false'
* String: A collection of characters

## Objects
Most times just a properties at the root level won't cut it, so objects are needed.
objects provide a local context for defining data, inside an object all values are paired with a key (like the root context). 
Objects are defined like so:
```
<object-name>:
	<property> = <value>
	...
```

### Additional details about objects
* Elements inside the object are unordered.
* Root is treated as an object 

## Lists
Sometimes the order of item is relavent, list provides an ordered but unnamed value container,
each value must start with a '*'

```
<list>:
	# For values
	* <item>
	# For object
	*
		<property> = <value>
		...
	# For list 
	*
		* <list-item>
		...
	...
```

## Complex/Long primitives:
Integers and floats can be grouped into groups of upto 15 elements (of the type), creating a long value
it can be done like so:

```<first>, <second>, ... <fifth_teenth>```

# Querys

## Get Property
```<owner_name>.<property_name>```
## Get Element of list
```<list_name>#<index>```
## Filter by type
```<object>:<type>...```

## Examples

Get the property name of object user: `.user.name`

Get the property name of object user only if its a string : `.user.name:string`

Get every property under user: `user.*`

Get every string property under user: `.user.*:string`

Get the first element of a list: `.user.list#0`

Get all strings from a all list in an object: `.user.*:list#*:string`
