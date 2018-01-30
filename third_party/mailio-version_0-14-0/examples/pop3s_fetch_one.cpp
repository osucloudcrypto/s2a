/*

pop3s_fetch_one.cpp
-------------------

Connects to POP3 server via SSL and fetches first message from mailbox.


Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <message.hpp>
#include <pop3.hpp>
#include <iostream>


using mailio::message;
using mailio::codec;
using mailio::pop3s;
using mailio::pop3_error;
using mailio::dialog_error;
using std::cout;
using std::endl;


int main()
{
    try
    {
        // mail message to store the fetched one
		int message_count = 9;
        message msg[message_count];
        // set the line policy to mandatory, so longer lines could be parsed
       
		
        // connect to server
        pop3s conn("pop.gmail.com", 995);
        // modify to use existing yahoo account
        conn.authenticate("recent:pop3testserver@gmail.com", "supersecretpassword", pop3s::auth_method_t::LOGIN);
        // fetch the first message from mailbox
		for (int i = 0; i <=message_count-1; i++)
		{
		msg[i].line_policy(codec::line_len_policy_t::MANDATORY);
        conn.fetch(i+1, msg[i]);	
        cout << msg[i].subject() << endl;
		cout << msg[i].date_time() << endl;
		cout<<msg[i].content() <<endl;
		}
		
	
    }
    catch (pop3_error& exc)
    {
        cout << exc.what() << endl;
    }
    catch (dialog_error& exc)
    {
        cout << exc.what() << endl;
    }

    return EXIT_SUCCESS;
}
