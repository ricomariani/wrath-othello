echo "native benchmark (the control)"
./wrath xlt endgame.txt
cd wrath-sharp
echo "wrath sharp on .NET 6.0"
dotnet run -c Release -f net6.0 xlt ../endgame.txt
echo "wrath sharp on .NET 7.0"
dotnet run -c Release -f net7.0 xlt ../endgame.txt
echo "wrath sharp on .NET 8.0"
dotnet run -c Release -f net8.0 xlt ../endgame.txt
cd ../wrath-sharp-inline-arrays
echo "wrath sharp on .NET 8.0 using inline arrays"
dotnet run -c Release xlt ../endgame.txt
