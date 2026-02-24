use rt::{compile, status};

def X: bool = true;

if (X) {
	fn add(x: int, y: int) int = x + y;

	let global = 1337;

	type coords = struct {
		x: int,
		y: int,
	};
} else {
	fn invalid() undefined_type = {
		return "hello" + 1337;
	};
};

def Q: int = :outer {
	{
		yield :outer, 42;
	};
};

if (true) {
	assert(Q == 42);
};

type platform = enum {
	LINUX,
	NETBSD,
	OPENBSD,
};

def PLATFORM = platform::LINUX;

let plat = switch (PLATFORM) {
case platform::LINUX =>
	yield "Linux";
case platform::NETBSD =>
	yield "NetBSD";
case platform::OPENBSD =>
	yield "OpenBSD";
};

fn invalid_cases() void = {
	// Non-static assert cannot reference symbols that are not yet defined
	compile(status::CHECK, `
	assert(X);
	def X: bool = true;
	`)!;
	compile(status::SUCCESS, `
	def X: bool = true;
	assert(X);
	`)!;

	// But static asserts are unordered
	compile(status::SUCCESS, `
	static assert(X);
	def X: bool = true;
	`)!;

	// Unchecked branches must still be valid syntax
	compile(status::PARSE, `
	if (false) {
		fn;
	};
	`)!;
};

export fn main() void = {
	assert(global == 1337);
	assert(add(2, 3) == 5);
	let x = coords { x = 10, y = 20 };
	assert(x.x == 10 && x.y == 20);
	assert(plat == "Linux");
	invalid_cases();
};
