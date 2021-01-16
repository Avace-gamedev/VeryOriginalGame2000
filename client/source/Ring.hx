// reference https://code.haxe.org/category/data-structures/ring-array.html
// reference https://github.com/torvalds/linux/blob/master/include/linux/circ_buf.h

@:generic
class Ring<T>
{
	var head:Int;
	var tail:Int;
	var cap:Int;
	var a:haxe.ds.Vector<T>;

	public function new(len)
	{
		if (len < 4)
		{
			len = 4;
		}
		else if (len & len - 1 > 0)
		{
			len--;
			len |= len >> 1;
			len |= len >> 2;
			len |= len >> 4;
			len |= len >> 8;
			len |= len >> 16;
			len++; // power of 2
		}
		cap = len - 1; // only "len-1" available spaces
		a = new haxe.ds.Vector<Null<T>>(len);
		reset();
	}

	public function reset()
	{
		head = 0;
		tail = 0;
	}

	// return i-th T from last
	public function get(i:Int)
	{
		if (i >= count())
			return null;

		return a[(head - i - 1) & cap];
	}

	public function push(v:T)
	{
		if (space() == 0)
			tail = (tail + 1) & cap;
		a[head] = v;
		head = (head + 1) & cap;
	}

	public inline function count()
		return (head - tail) & cap;

	public inline function space()
		return (tail - head - 1) & cap;
}

@:generic
class PositionalRing<T>
{
	var head:Int;
	var tail:Int;
	var cap:Int;
	var a:haxe.ds.Vector<Null<T>>;

	public var head_id(get, null):Int;
	public var tail_id(get, null):Int;

	public function new(len)
	{
		if (len < 4)
		{
			len = 4;
		}
		else if (len & len - 1 > 0)
		{
			len--;
			len |= len >> 1;
			len |= len >> 2;
			len |= len >> 4;
			len |= len >> 8;
			len |= len >> 16;
			len++; // power of 2
		}
		cap = len - 1; // only "len-1" available spaces
		a = new haxe.ds.Vector<Null<T>>(len);
		reset();
	}

	public function reset()
	{
		head = 0;
		tail = 0;
	}

	public function get_tail_id()
	{
		if (count() <= 0)
			return -1;
		return head_id - count();
	}

	public function get_head_id()
	{
		if (count() <= 0)
			return -1;
		return head_id;
	}

	public function getById(id:Int)
	{
		if (count() <= 0 || id > head_id || id < head_id - cap)
			return null;

		return a[(head - (head_id - id) - 1) & cap];
	}

	// return Bytes where 1 in pos k means T k is not null
	// position 0 is the most recent T
	public function makeAck()
	{
		var n_bytes = Math.ceil(cap / 8);
		var ack = haxe.io.Bytes.alloc(n_bytes);
		for (i in 0...n_bytes)
		{
			var cur_byte = 0;
			for (j in 0...8)
				if (get(i * 8 + j) != null)
					cur_byte |= 1 << j;
			ack.set(i, cur_byte);
		}

		return ack;
	}

	// return i-th T from last
	public function get(i:Int)
	{
		if (i >= count())
			return null;

		return a[(head - i - 1) & cap];
	}

	public function write(id:Int, v:T)
	{
		if (count() == 0)
		{
			push(v);
			head_id = id;
		}
		else
		{
			if (id > head_id)
			{
				for (i in 0...(id - head_id - 1))
					skip();
				push(v);
				head_id = id;
			}
			else if (id >= head_id - cap)
			{
				a[(head - (head_id - id) - 1) & cap] = v;
				if (head_id - id > count())
				{
					tail = (head - (head_id - id) - 1) & cap;
				}
			}
		}
		return v;
	}

	function push(v:T)
	{
		if (space() == 0)
			tail = (tail + 1) & cap;
		a[head] = v;
		head = (head + 1) & cap;
	}

	function skip()
		push(null);

	public function toString()
	{
		var ackByte = makeAck();
		var ack = '';
		for (i in 0...ackByte.length)
			ack = Utils.byteToBinaryString(ackByte.get(i)) + ack;

		return '[head_id:$head_id, tail_id:$tail_id, ack:$ack]';
	}

	public inline function count()
		return (head - tail) & cap;

	public inline function space()
		return (tail - head - 1) & cap;
}
