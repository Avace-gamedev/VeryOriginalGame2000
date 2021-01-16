import Types.NetworkFrame;
import haxe.io.Bytes;
import haxe.io.BytesBuffer;
#if sys
import sys.net.Host;
import sys.net.Socket;
import sys.net.UdpSocket;
#end

class UDPClient
{
	var sock(default, null):UdpSocket;

	var last_sent_init:Float = -10;
	var init_timeout:Int = 5;

	var buffer:BytesBuffer;
	var msg_queue:Array<NetworkFrame> = [];

	public var ID:Int;
	public var server_timeout:Int; // in ms
	public var initialized:Bool = false;

	var last_sent_message:Float = -1; // any kind of message, this is used to send pings to avoid timeouts
	var ping:NetworkFrame = {opcode: Config.OP_PING, size: 0, content: haxe.io.Bytes.alloc(0)};
	var pong:NetworkFrame = {opcode: Config.OP_PONG, size: 0, content: haxe.io.Bytes.alloc(0)};

	public var connection_attempts:Int = 0;

	public function new(addr:String, port:Int)
	{
		buffer = new BytesBuffer();

		var host = new Host(addr);
		sock = new UdpSocket();
		sock.connect(host, port);
		sock.setTimeout(0.01);
	}

	public function doInit()
	{
		if (Sys.time() > last_sent_init + init_timeout)
		{
			trace('Trying to connect... ' + (connection_attempts > 0 ? '($connection_attempts)' : ""));

			write({
				opcode: Config.OP_INIT,
				size: 7,
				content: Bytes.ofString("hithere"),
			});

			last_sent_init = Sys.time();
			connection_attempts++;
		}
	}

	public function write(frame:NetworkFrame)
	{
		var bytes = Utils.bytesOfNetworkFrame(frame);
		last_sent_message = Sys.time() * 1000;
		return sock.output.writeBytes(bytes, 0, bytes.length);
	}

	public function hasMessages()
		return msg_queue.length > 0;

	public function popMessage()
		return msg_queue.pop();

	public function queue(msg:NetworkFrame)
		msg_queue.unshift(msg);

	public function update()
	{
		// send ping if needed
		if (initialized && Sys.time() * 1000 > last_sent_message + server_timeout / 10)
		{
			write(ping);
			last_sent_message = Sys.time() * 1000;
		}

		// read messages
		var actually_read = -1;
		var local_buffer = Bytes.alloc(1024);

		try
		{
			actually_read = sock.input.readBytes(local_buffer, 0, 1024);
		}
		catch (Broken) {};

		if (actually_read > 0)
		{
			var actual_buffer;
			var actual_buffer_size;
			if (buffer.length > 0)
			{
				trace("buffer.length = " + buffer.length);
				var bytes = buffer.getBytes();
				actual_buffer = Bytes.alloc(bytes.length + actually_read);
				actual_buffer.blit(0, bytes, 0, bytes.length);
				actual_buffer.blit(bytes.length, local_buffer, 0, actually_read);
				actual_buffer_size = bytes.length + actually_read;

				buffer = new BytesBuffer();
			}
			else
			{
				actual_buffer = local_buffer;
				actual_buffer_size = actually_read;
			}

			var start = 0;
			while (actual_buffer_size - start > 0)
			{
				var read_bytes = tryReadMessage(actual_buffer, start, actually_read);
				if (read_bytes > 0)
					start += read_bytes
				else if (read_bytes < 0)
					break;
				else
					start += 1;
			}

			if (actual_buffer_size - start > 0)
			{
				buffer.addBytes(actual_buffer, start, (actual_buffer.length - start));
			}
		}
	}

	function tryReadMessage(buff:Bytes, start:Int, end:Int)
	{
		var opcode = buff.get(start);
		var expected_len = buff.getInt32(start + 1);
		if (expected_len <= 0) // len 0, weird, update will continue reading
			return 0;
		else if (expected_len > (end - start - 5))
			return -1;
		else
		{
			switch (opcode)
			{
				case Config.OP_PING:
					write(pong);
				case Config.OP_PONG:
				// nothing
				case Config.OP_INIT:
					if (!initialized)
						if (expected_len >= 7)
						{
							var str = buff.sub(5, 7).toString();

							if (str == "hithere")
							{
								ID = buff.getInt32(12);
								server_timeout = buff.getInt32(16);
								initialized = true;

								trace("Got ID " + ID);

								connection_attempts = 0;
							}
							else
								trace("bad server init");
						}
						else
							trace("bad server init");
					else
						trace("already initialized");
				// don't add the following frames, they should not be received by the client
				case Config.OP_CLIENT_READY:
					trace("received client ready ??");
				case Config.OP_CONTROL_FRAME:
					trace("received control frame ??");
				default:
					var frame = {
						opcode: opcode,
						size: expected_len,
						content: buff.sub(start + 5, expected_len),
					};
					queue(frame);
			}
			return expected_len + 3;
		}
	}
}
