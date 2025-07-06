fn test() int = {
	const x = 2;
	const add_x_plus_arg_plus_2 = fn(y: int) int => {
		return (fn(z: int) int => x + y + z)(2);
	};
	return add_x_plus_arg_plus_2(2);
};
