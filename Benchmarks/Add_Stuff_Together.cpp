#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>


using namespace std;

int main(int argc, char * argv[])
{
    std::fstream myfile("Add_Benchmarks.txt", std::ios_base::in);

    float a = 0;

    myfile >> a;

    while (!myfile.fail())
    {
        printf("%f ", a);
        myfile >> a; // here you dispay your numbers
    }

   // getchar(); // waiting for input

    long double sum = 0;
    long double x(0);
    long double sumcount(0);
    long double average = 0;
    int even(0);
    int odd(0);

    ifstream fin;

    string file_name;

    cout<<"numbers.txt" << endl;

   // cin>> file_name; // waiting for enter file name

    fin.open("Add_Benchmarks.txt",ios::in);

    if (!fin.is_open())
    {
        cerr<<"Unable to open file "<<file_name<<endl;
        exit(10);
    }

    fin >> x;

    while (!fin.fail())
    {
        cout<<"Read integer: "<<x<<endl; // display number again

        sum=sum+x;
        sumcount++;
       /* if(x % 2==0)  // compuing your file statistics
            even++;
        else
            odd++;
*/
        fin>>x;
    }

    fin.close();
    average=(double long)sum/100;
	cout.precision(20);
    cout<<"Sum of integers: "<<sum<<endl; // displaying results
    cout<<"Average: "<<average<<endl;
    cout<<"Number of even integers: "<<even<<endl;
    cout<<"Number of odd integers: "<<odd<<endl;

    return 0;
}