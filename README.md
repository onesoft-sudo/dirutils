# Dirutils

Dirutils is a collection of utility program for working with directories.

Currently these programs are available:

- `dirscan` - Scans the given directories and prints out the files inside of them 
- `dirstats` - Shows statistical information about the given directory.
- `dirwatch` - Watches for any changes in the given directory.

Run `program --help` to see a short documentation of `program`, where `program` is 
the name of the program that you want to invoke.

Homepage: <https://dirutils.nongnu.org>

## Download 

Latest and old source release tarballs can be found at <https://download.savannah.gnu.org/releases/dirutils/>.

## Building and installation

If you have downloaded the source release tarball, navigate to the directory first and then run the following commands:

```
tar -xvf dirutils-x.y.z.tar.gz 
cd dirutils-x.y.z/
./configure
make
make install
```

Replace `x.y.z` with the version number of dirutils that you've downloaded. 

`make install` might fail if the current user does not have permission to make changes to the system directories. To fix this, run `sudo make install`. This will give root permissions to `make` and thus, it will be able to do it's job successfully. This command might ask you for your password.

Then you can run `dirstats --version` to check if it's working properly. You should see a version number being printed to the screen. 

## Building from Git repository

Note that this requires you to have Git installed in your local computer. If not, go to <https://git-scm.com/> to download and install Git. You'll also need `autoconf` and `automake` to build the package. Go to <https://gnu.org/software/autoconf> and <https://gnu.org/software/automake> to download and install them.

To clone the repository, run:

```
git clone git://git.savannah.gnu.org/dirutils.git
```

This should create a directory `dirutils`. Run `cd dirutils` to go inside of the directory.

The repository of dirutils does not come with the build scripts (e.g. `configure`). To generate those scripts, run:

```
autoreconf -i
```

It should create `configure`, `build-aux/` and a few other files.

Then run:

```
make
make install
```

Again, `make install` might fail if the current user does not have permission to make changes to the system directories. You should run `sudo make install` to fix this.


## License

Dirutils is licensed under GNU General Public License v3+. You should have received a copy with this repository/release. If not, see <https://www.gnu.org/licenses/gpl-3.0.en.html>.

## Development and maintenance

Dirutils is maintained by Ar Rakin <rakinar2@onesoftnet.eu.org>.
