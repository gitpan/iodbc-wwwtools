SELECT pe_id, pe_lname, pe_fname
   FROM person;
;
<HTML>
<HEAD>
<TITLE>Template Sample</TITLE>
</HEAD>
<BODY>
This is an example of what can be done with templates. A template is
a text file with five segments, each separated by a semicolon. The
first segment is an SQL Statement. When it is issued, the column
names magically become available as perl \$variables in the template.<BR>
<STRONG>Example:</STRONG><BR>
<PRE>
SELECT pe_lname, pe_fname
   FROM sometable;<P> 
</PRE>

The next segment is the data type declaration. See the documentation
on the embedded SQL for how this works. You only need this if you are
including form variables in your query.<P>
<STRONG>Example:</STRONG><BR>
<PRE>
pe_name=CHAR
pe_id=CHAR
pe_bday=DATE;<P>
</PRE>

What follows now is the HTML, which is broken out into three segments:
the header, the detail, and the footer. The header must contain the usual
HTML head stuff, but can (and usually should) contain HTML body elements.
The detail portion contains the HTML you want to print for each record,
and usually has one or more \$variables. The footer is the last stuff that
prints and should, at a minimum, close the BODY and HTML tags.<P>
<STRONG>Header Example:</STRONG><BR>
<PRE>
&lt;HTML&gt; 
&lt;HEAD&gt; 
&lt;TITLE&gt;Sample&lt;/TITLE&gt; 
&lt;/HEAD&gt; 
&lt;BODY&gt; 
&lt;H1&gt;Welcome to my sample&lt;/H1&gt;;<P>
</PRE>

<STRONG>Detail Example:</STRONG><BR>
<PRE>
Their name is \$pe_fname \$pe_lname &lt;P&gt;;<P>
</PRE>

<STRONG>Footer Example:</STRONG><BR>
<PRE>
&lt;/BODY&gt; 
&lt;/HTML&gt; 
</PRE>

Finally, here's an HTML table, built using a template:<P>
<TABLE BORDER>
<TH>Primary Key</TH><TH>Last Name</TH><TH>First Name</TH>;

<TR>
<TD>$pe_id</TD>
<TD>$pe_lname</TD>
<TD>$pe_fname</TD>
</TR>;

</TABLE>
</BODY>
</HTML>

