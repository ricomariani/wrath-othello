echo "native benchmark (the control)"
./wrath xlt midgame.txt
cd wrath-sharp
echo "wrath sharp on .NET 6.0 using Vector128"
dotnet run -c Release -f net6.0 xlt ../midgame.txt
echo "wrath sharp on .NET 7.0 using Vector128"
dotnet run -c Release -f net7.0 xlt ../midgame.txt
echo "wrath sharp on .NET 8.0 using Vector128"
dotnet run -c Release -f net8.0 xlt ../midgame.txt
cd ../wrath-sharp-inline-arrays
echo "wrath sharp on .NET 8.0 using inline arrays"
dotnet run -c Release xlt ../midgame.txt
cd ../wrath-sharp-std-arrays
echo "wrath sharp on .NET 6.0 standard arrays"
dotnet run -c Release -f net6.0 xlt ../midgame.txt
echo "wrath sharp on .NET 7.0 standard arrays"
dotnet run -c Release -f net7.0 xlt ../midgame.txt
echo "wrath sharp on .NET 8.0 standard arrays"
dotnet run -c Release -f net8.0 xlt ../midgame.txt
