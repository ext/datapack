# Datapack samples

## Method 1: handles

Files packed into binary and read using handle. This is the easiest method and requires almost no changes to application.

## Method 2: virtual filenames

Files packed into binary but read using virtual filenames. If built into an executable file the `filetable` symbol must be accessable (e.g. build with `-rdynamic` flag). This method allows files to be overridden by the user (if enabled) by creating a file with the same path.

## Method 3: binary blob

Files is packed into a binary blob which is distributed along with the application but otherwise works similar to method 2.
