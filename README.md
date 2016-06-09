Adaptative Code Refinement
==========================

Overview
--------

ACR is a compiler technique combining code instrumentation and a runtime library
which will automaticaly optimize a computation kernel at runtime by analysing
the dataset.

### How does it works?

By instrumenting your code with easy to use pragmas, you will provide the tool
with enouth information for it to do it's job.

Given the following stencil kernel doing a mean in a "plus" shape:
~~~{.c}
double data[M][N];

for (int k = 0; k < P; ++k)
  for (int i = 0; i < M-2; ++i)
    for (int j = 0; j < N-2; ++j)
      data[i+1][j+1] +=
        (data[i][j+1] + data[i+1][j] + data[i+1][j+2] + data[i+2][j+1]) / 4.;
~~~

Pretend this kernel, a 2D Von Neumann neighbourhood applied P times, is part of
a simulation to smoothen the data after an other algorithm and that both are
called many times.

If the neighbourhood of the data is between a range of delta you have chosen
wisely, you obviously does not need to smoothen them further. This is where ACR
shines, you can ask him to monitor the "data" array and change the value of the
"P" parameter. This way you can lower the computation overhead by doing less
pass of this algorithm where it does not matter that much.

So the by adding the following pragmas, the kernel will automatically generate
an optimized kernel depending on the data.

~~~{.c}
double data[M][N];

#pragma acr grid(25)
#pragma acr monitor(double data[i][j], avg, data_to_strategy)
#pragma acr alternative high (parameter, P = 5)
#pragma acr alternative low  (parameter, P = 1)
#pragma acr strategy direct (0, high)
#pragma acr strategy direct (1, low)
for (int k = 0; k < P; ++k)
  for (int i = 0; i < M-2; ++i)
    for (int j = 0; j < N-2; ++j)
      data[i+1][j+1] +=
        (data[i][j+1] + data[i+1][j] + data[i+1][j+2] + data[i+2][j+1]) / 4.;
~~~

The data_to_strategy is a function matching the data value to a strategy unsigned
integer one.

Build
-----

~~~ {.bash}
mkdir build && cd build
cmake ..
make
~~~

Documentation
-------------

Make sure [Doxygen](http://doxygen.org) is installed on your system.

- Fedora
  ~~~{.bash}
  sudo dnf install doxygen
  ~~~
- ArchLinux
  ~~~{.bash}
  sudo pacman -S doxygen
  ~~~
- Debian / Ubuntu
  ~~~{.bash}
  sudo apt-get install doxygen
  ~~~

To build the documentation:

~~~ {.bash}
mkdir build && cd build
cmake ..
make doxygen
~~~

After that you will find the generated documentation as ``html`` or ``latex``
format in the ``doc`` folder.

License
-------

Copyright (C) 2016 Maxime Schmitt

ACR is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.
