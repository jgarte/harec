use rt::{compile, status};

fn simple() void = {
	let add: *fn[int, int] int = fn[x: int, y: int] int => x + y;
	assert(add(2, 2) == 4);
};

fn captures() void = {
	let x = 1337;
	(fn[] void => {
		assert(x == 1337, "variables should capture from the environment");
	})();

	const q = fn[] void => assert(x == 1337, "variables are captured by value");
	x = 4242;
	q();

	// Nested captures
	const q = fn[] void => {
		let q = fn[] void => assert(x == 4242);
		x = 2424;
		q();
		(fn[] void => assert(x == 2424))();
	};
	x = 1337;
	q();

	// Captured references should work though
	{
		let x = &x;
		(fn[] void => *x = 4242)();
	};
	assert(x == 4242);
};

fn invalid() void = {
	// Function declarations cannot be closures
	compile(status::PARSE, "
	fn test[] void = {
		void;
	};
	")!;

	// And vice-versa
	compile(status::PARSE, "
	fn test() void = {
		let x = fn() void => void;
	};
	")!;
};

export fn main() void = {
	simple();
	captures();
	invalid();
};
