Coding Guidelines
=================

Each function should verify the validity of its arguments, at least to some
extent. If the function is a part of the public interface, it must exit with
AUSHAPE_RC_INVALID_ARGS, if arguments are invalid. Other functions should
assert the validity of their arguments.

Each function modifying objects must assert the validity of all modified
objects on exit.
