export fn memcpy(dest: *opaque, src: *opaque, amt: size) void = {
	let a = dest: *[*]u8;
	let b = src: *[*]u8;
	for (let i = 0z; i < amt; i += 1) {
		a[i] = b[i];
	};
};
