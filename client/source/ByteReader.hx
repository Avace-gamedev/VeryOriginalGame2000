class ByteReader
{
	var offset:Int = 0;
	var size:Int;

	var buffer:haxe.io.Bytes;

	public function new(buffer:haxe.io.Bytes)
	{
		this.buffer = buffer;
		startReading();
	}

	public function startReading()
		size = 0;

	public function nByteRead()
		return size;

	public function seek(i:Int)
	{
		offset = i;
	}

	public function readInt32()
	{
		#if debug
		if (offset + 4 > buffer.length)
			trace('readInt32: cannot read at $offset from buffer of length ' + buffer.length);
		#end

		var res = buffer.getInt32(offset);
		offset += 4;
		size += 4;

		return res;
	}

	public function readInt64()
	{
		#if debug
		if (offset + 8 > buffer.length)
			trace('readInt64: cannot read at $offset from buffer of length ' + buffer.length);
		#end

		var res = buffer.getInt64(offset);
		offset += 8;
		size += 8;

		return res;
	}

	public function readFloat()
	{
		#if debug
		if (offset + 4 > buffer.length)
			trace('readFloat: cannot read at $offset from buffer of length ' + buffer.length);
		#end

		var res = buffer.getFloat(offset);
		offset += 4;
		size += 4;

		return res;
	}

	public function readUInt16()
	{
		#if debug
		if (offset + 2 > buffer.length)
			trace('readUInt16: cannot read at $offset from buffer of length ' + buffer.length);
		#end

		var res = buffer.getUInt16(offset);
		offset += 2;
		size += 2;

		return res;
	}

	public function readInt8()
	{
		#if debug
		if (offset + 1 > buffer.length)
			trace('readInt8: cannot read at $offset from buffer of length ' + buffer.length);
		#end

		var res = buffer.get(offset);
		offset += 1;
		size += 1;

		return res;
	}

	public function readBytes(len:Int)
	{
		#if debug
		if (offset + len > buffer.length)
			trace('readBytes: cannot read at $offset from buffer of length ' + buffer.length);
		#end

		var res = haxe.io.Bytes.alloc(len);
		res.blit(0, buffer, offset, len);
		offset += len;
		size += len;

		return res;
	}

	public function readString()
	{
		var i = 0;
		while (buffer.get(offset + i) != 0)
			i++;

		var str = buffer.sub(offset, i).toString();
		offset += i + 1;
		size += i + 1;

		return str;
	}
}
