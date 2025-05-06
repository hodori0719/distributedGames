# Scaling Virtual Worlds: A Distributed Server Architecture for Real-Time Multiplayer Games
# CS490 Senior Thesis, June Yoo. Adviser, Michael Shah

## Overview
Details of the project are included in the report at https://zoo.cs.yale.edu/classes/cs490/24-25b/yoo.june.jhy24/yoo_final_report.pdf

# Usage
To avoid message passing between processes, optimal usage of this code requires the application to implement library-specific logic within `lib/game/game.go`, following the interface specified in the code.

Servers assume that a locator exists. Demo locator & server implementations using the library are provided in `demo`. Each must be run separately or using the scripts provided in `scripts`. Code is also provided to interface with the server in `demo/client`.

## Demo
Visualization + short demo (1:30): https://drive.google.com/file/d/1dHoqU7HeuzJvZQHsFV4LancT8UYyy8_0/view?usp=sharing
Full demo (13:20): https://drive.google.com/file/d/1b3cQfjr3c_X6CdkeGwb2SyPQkK9wxsgR/view?usp=sharing

# Additional Materials
[Slide deck](https://docs.google.com/presentation/d/1ZTPcmTSY3gRpwFjcTB_f3mWait5_L4oEMBwp0IQOTvk/edit?usp=sharing) for a 1 hour presentation I did on this topic.