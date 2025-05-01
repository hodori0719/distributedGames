#!/usr/bin/perl
#
# This is very hacky and only works with a particular text format.
# The interface should look like:
#
# class $interface ... {
#
# virtual $ret $method ($t1 $a1, ...) = 0;
#
# };
#
#
# Usage:
#
# make_stub.pl -p <output_file_prefix> 
#              -i <interface_class_name>
#              -f <interface_include_file_name>
#              <input_file>

# We assume:
#
# * no extra line breaks (each pure virtual method on one line)
# * no comments in the line
# * no commas in type names (including templates) xxx fixme
# * no spaces in type names (including templates) xxx fixme
# * only single pointers or references. (i.e., no type **, or type *&)
# * types ending in Type are enums (serialized as integers)
# * all pointer values are autoserializable (implement Clone)
# * all pointer types are of the base autoserializable type (i.e., Event)
# * all non-basic types are serializable and have copy constructors
# * only pure virtual methods get stubs.

use strict;

use lib "$ENV{HOME}/Merc/run";
use lib "$ENV{HOME}/Merc/run/lib";
use Travertine;
use IO::File;
use Getopt::Std;
use vars qw($opt_p $opt_i $opt_f $opt_s);

getopts("p:i:f:s:");

my $prefix    = $opt_p || die "need prefix";
my $interface = $opt_i || die "need interface";
my $inclfile  = $opt_f || die "need include file";
my %skipfuncs = map { $_ => 1 } split(/,/, $opt_s);

my $in_interface = 0;

my @methods;

while (<>) {
    if (/^\s*class\s+$interface\b.*\{/) {
	$in_interface = 1;
    }
    if ($in_interface &&
	/^\s*virtual\s+((?:const )?[^ \t]+)\s+([^ \t]+)\s*\(([^\)]*)\)\s*=\s*0\s*\;/) {
	my ($ret, $method, $args) = ($1, $2, $3);
	my @args;
	foreach my $t (split(/,/, $args)) {
	    if ($t =~ /^\s*(.*?)\s*\b(\w+)$/) {
		my ($type, $arg) = ($1, $2);
		$type =~ s/\s+/ /g;

		push @args, [$type, $arg];
	    } else {
		twarn "bad arg: $t ($ret,$method,$args)";
	    }
	}
	
	push @methods, [$ret, $method, \@args];
    }
    if ($in_interface && /\};/) {
	last;
    }
}

if (!$in_interface) {
    tdie "didn't find $interface";
}

###############################################################################

sub typeBase {
    my $t = shift;
    $t =~ s/(\*|\&)//g;
    $t =~ s/\s+$//g;
    $t =~ /([^ ]+)$/;
    return $1;
}

sub typeIsRef {
    my $t = shift;
    return ($t =~ /\&/);
}

sub typeIsPtr {
    my $t = shift;
    return $t =~ /\*/;
}

###############################################################################

sub make_rpc {
    my $fname = "$prefix\_MercRPC";
    my $fname_caps = uc $fname;
    $fname_caps =~ s|/|_|g;

    # header
    {
	my $out = new IO::File(">$fname.h") or tdie $!;
	print $out "#ifndef __$fname_caps\_H_\n";
	print $out "#define __$fname_caps\_H_\n\n";
	
	print $out "#include <rpc/MercRPC.h>\n";
	print $out "#include <$inclfile>\n\n";
	
	make_hdr_msgs($out, @methods);
	make_hdr_srv_stub($out, @methods);
	make_hdr_cli_stub($out, @methods);
	
	print $out "#endif /* $fname_caps */\n";
	
	$out->close();
    }

    # impl
    {
	my $out = new IO::File(">$fname.cpp") or tdie $!;
	print $out "#include <mercury/AutoSerializer.h>\n";
	print $out "#include <rpc/MercRPCMarshaller.h>\n";
	print $out "#include \"$fname.h\"\n\n";

	make_impl_msgs($out, @methods);
	make_impl_srv_stub($out, @methods);
	make_impl_cli_stub($out, @methods);

	$out->close();
    }
    
}

###############################################################################

sub make_hdr_srv_stub {
    my $out = shift;
    my @methods = @_;

    my $name = "$interface\_ServerStub";

    print $out <<EOT;
class $name : public virtual MercRPCServerStub {
  private:
    $interface *iface;
  public:
    $name($interface *iface) : iface(iface) {}
    bool Dispatch(MercRPC *rpc, MercRPCMarshaller *m);
};

EOT
}

sub make_impl_srv_stub {
    my $out = shift;
    my @methods = @_;
    
    my $name = "$interface\_ServerStub";

    print $out <<EOT;
bool $name\::Dispatch(MercRPC *rpc, MercRPCMarshaller *m) {
    int type = rpc->GetType();
    if (0) {}
EOT
    foreach my $m (@methods) {
	next if $skipfuncs{$m->[1]};
	my ($ret, $method, $args) = @$m;
	my $rpc_cname = "MercRPC\_$interface\_$method";
	my $rpc_tname = uc $rpc_cname;
	my $res_cname = "MercRPC\_$interface\_$method\_Result";
	my $res_tname = uc $res_cname;

	print $out <<EOT;
    else if (type == $rpc_tname) {
	$rpc_cname *mrpc = dynamic_cast<$rpc_cname *>(rpc);
EOT
        if ($ret ne 'void') {
	    print $out "         ";
	    print $out "$res_cname res(mrpc, iface->$method(";
	    my $fst = 1;
	    foreach my $a (@$args) {
		my ($t, $n) = @$a;
		print $out ", " unless $fst; $fst = 0;
		print $out "mrpc->$n";
	    }
	    print $out "));\n";
	} else {
	    print $out "         ";
	    print $out "$res_cname res(mrpc);\n";
	    print $out "         ";
	    print $out "iface->$method(";
	    my $fst = 1;
	    foreach my $a (@$args) {
		my ($t, $n) = @$a;
		print $out ", " unless $fst; $fst = 0;
		print $out "mrpc->$n";
	    }
	    print $out ");\n";
	}
        print $out <<EOT;
         m->HandleRPC(rpc, &res);
	 return true;
    }
EOT
    }
    print $out <<EOT;
    else {
	return false;
    }
}

EOT
}

###############################################################################

sub make_hdr_cli_stub {
    my $out = shift;
    my @methods = @_;

    my $iname = "$interface\_ClientStub";

    print $out <<EOT;
class MercRPCMarshaller;

class $iname : public $interface {
  private:
    static $iname *m_Instance;
    MercRPCMarshaller *m;
  protected:
    $iname(MercRPCMarshaller *m) : m(m) {}
  public:
    static $iname *GetInstance();
    virtual ~$iname() {}
EOT
    foreach my $m (@methods) {
	my ($ret, $method, $args) = @$m;

	print $out "    ";
	print $out "$ret $method (";
	my $fst = 1;
	foreach my $a (@$args) {
	    my ($t, $n) = @$a;
	    print $out ", " unless $fst; $fst = 0;
	    print $out "$t $n";
	} 
	print $out ");\n";
    }
    print $out <<EOT;
};

EOT

}

sub make_impl_cli_stub {
    my $out = shift;
    my @methods = @_;

    my $iname = "$interface\_ClientStub";

    print $out "$iname *$iname\::m_Instance = NULL;\n\n";
    print $out <<EOT;
$iname *$iname\::GetInstance() {
    if (m_Instance == NULL) {
	m_Instance =
	    new $iname(MercRPCMarshaller::GetInstance());
    }
    return m_Instance;
}

EOT
    foreach my $m (@methods) {
	my ($ret, $method, $args) = @$m;
	my $rpc_cname = "MercRPC\_$interface\_$method";
	my $rpc_tname = uc $rpc_cname;
	my $res_cname = "MercRPC\_$interface\_$method\_Result";
	my $res_tname = uc $res_cname;

	my $default = typeDefaultVal($ret);

	print $out "$ret $iname\::$method (";
	my $fst = 1;
	foreach my $a (@$args) {
	    my ($t, $n) = @$a;
	    print $out ", " unless $fst; $fst = 0;
	    print $out "$t $n";
	} 
	print $out ") {\n";

	if ($skipfuncs{$method}) {
	    print $out "   return $default;\n";
	    print $out "}\n\n";
	    next;
	}

	if (@$args > 0) {
	    print $out "   $rpc_cname rpc(";
	    $fst = 1;
	    foreach my $a (@$args) {
		my ($t, $n) = @$a;
		print $out ", " unless $fst; $fst = 0;
		print $out "$n";
	    } 
	    print $out ");\n";
	} else {
	    print $out "   $rpc_cname rpc;";
	}
	print $out "   MercRPCResult *res = m->Call(&rpc);\n";
	print $out "   if (!res) return $default;\n";
	print $out "   ASSERT(res && !res->Error() && res->GetType() == $res_tname);\n";
	if ($ret ne 'void') {
	    print $out "   $res_cname *tres = dynamic_cast<$res_cname *>(res);\n";
	    print $out "   return tres->res;\n";
	} else {
	    print $out "   return;\n";
	}
	print $out "}\n\n";
    }
}

###############################################################################

sub make_hdr_msgs {
    my $out = shift;
    my @methods = @_;

    print $out "void $interface\_RegisterTypes();\n\n";
	
    foreach my $m (@methods) {
	next if $skipfuncs{$m->[1]};
	make_rpc_method($out, $m, 1);
	make_res_method($out, $m, 1);
    }
}

sub make_impl_msgs {
    my $out = shift;
    my @methods = @_;
    
    foreach my $m (@methods) {
	next if $skipfuncs{$m->[1]};
	my ($ret, $name, $args) = @$m; 
	{
	    my $cname = "MercRPC\_$interface\_$name";
	    my $tname = uc $cname;
	    print $out "MsgType $tname;\n";
	}
	{
	    my $cname = "MercRPC\_$interface\_$name\_Result";
	    my $tname = uc $cname;
	    print $out "MsgType $tname;\n";
	}
    }
    print $out "\n";
    
    print $out "void $interface\_RegisterTypes() {\n";
    foreach my $m (@methods) {
	next if $skipfuncs{$m->[1]};
	my ($ret, $name, $args) = @$m;
	{
	    my $cname = "MercRPC\_$interface\_$name";
	    my $tname = uc $cname;
	    
	    print $out "   $tname =\n      REGISTER_TYPE(Message,$cname);\n";
	}
	{
	    my $cname = "MercRPC\_$interface\_$name\_Result";
	    my $tname = uc $cname;
	    
	    print $out "   $tname =\n      REGISTER_TYPE(Message,$cname);\n";
	}
    }
    print $out "}\n\n";
}

sub make_res_method {
    my $out = shift;
    my $method = shift;
    my $isrpc = shift;
    my ($ret, $name, $args) = @$method;
    my @args = @$args;

    my $cname = "MercRPC\_$interface\_$name\_Result";
    my $tname = uc $cname;

    print $out "extern MsgType $tname;\n";

    print $out "struct $cname : public MercRPCResult {\n";
    print $out "   DECLARE_TYPE(Message, $cname);\n";
    # return type
    if ($ret ne 'void') {
	my $b = typeBase($ret);
	my $s = typeIsPtr($ret) ? "*" : "";
	print $out "   $b ${s}res;\n";
    }
    # normal constructor
    print $out "\n";
    print $out "   $cname(";
    {
	print $out "MercRPC *req";
	if ($ret ne 'void') {
	    print $out ", $ret res";
	}
    }
    print $out ") : ";
    {
	if ($ret ne 'void') {
	    print $out "res(res)";
	}
    }
    print $out ", " unless $ret eq 'void';
    print $out "MercRPCResult(req) {}\n";
    # serializable constructor
    print $out "   $cname(Packet *pkt) : MercRPCResult(pkt) {\n";
    {
	if ($ret ne 'void') {
	    print $out "      ";
	    deserialize($out, $ret, "res", "pkt");
	}
    }
    print $out "   }\n";
    # serialize
    print $out "   void Serialize(Packet *pkt) {\n";
    print $out "      MercRPCResult::Serialize(pkt);\n";
    {
	if ($ret ne 'void') {
	    print $out "      ";
	    serialize($out, $ret, "res", "pkt");
	}
    }
    print $out "   }\n";
    # getlength
    print $out "   uint32 GetLength() {\n";
    print $out "      int len = MercRPCResult::GetLength();\n";
    {
	if ($ret ne 'void') {
	    print $out "      ";
	    getlength($out, $ret, "res", "len");
	}
	print $out "      return len;\n";
    }
    print $out "   }\n";
    # type string
    print $out "   const char* TypeString() { return \"$tname\"; }\n";
    print $out "};\n\n";
}

sub make_rpc_method {
    my $out = shift;
    my $method = shift;
    my $isrpc = shift;
    my ($ret, $name, $args) = @$method;
    my @args = @$args;

    my $cname = "MercRPC\_$interface\_$name";
    my $tname = uc $cname;

    print $out "extern MsgType $tname;\n";

    print $out "struct $cname : public MercRPC {\n";
    print $out "   DECLARE_TYPE(Message, $cname);\n";
    # member variables
    foreach my $a (@args) {
	my ($t, $n) = @$a;
	my $b = typeBase($t);
	my $s = typeIsPtr($t) ? "*" : "";
	print $out "   $b $s$n;\n";
    }
    # normal constructor
    print $out "\n";
    print $out "   $cname(";
    {
	my $fst = 1;
	foreach my $a (@args) {
	    my ($t, $n) = @$a;
	    print $out ", " unless $fst; $fst = 0;
	    print $out "$t $n";
	}
    }
    print $out ") : ";
    {
	my $fst = 1;
	foreach my $a (@args) {
	    my ($t, $n) = @$a;
	    print $out ", " unless $fst; $fst = 0;
	    my $c;
	    if (typeIsPtr($t)) {
		$c = "$n->Clone()";
	    } else {
		$c = $n;
	    }
	    print $out "$n($c)";
	}
    }
    print $out ", " unless @args == 0;
    print $out "MercRPC((uint32)0) {}\n";
    # serializable constructor
    print $out "   $cname(Packet *pkt) : MercRPC(pkt) {\n";
    {
	foreach my $a (@args) {
	    my ($t, $n) = @$a;
	    print $out "      ";
	    deserialize($out, $t, $n, "pkt");
	}
    }
    print $out "   }\n";
    # serialize
    print $out "   void Serialize(Packet *pkt) {\n";
    print $out "      MercRPC::Serialize(pkt);\n";
    {
	foreach my $a (@args) {
	    my ($t, $n) = @$a;
	    print $out "      ";
	    serialize($out, $t, $n, "pkt");
	}
    }
    print $out "   }\n";
    # getlength
    print $out "   uint32 GetLength() {\n";
    print $out "      int len = MercRPC::GetLength();\n";
    {
	foreach my $a (@args) {
	    my ($t, $n) = @$a;
	    print $out "      ";
	    getlength($out, $t, $n, "len");
	}
	print $out "      return len;\n";
    }
    print $out "   }\n";
    # type string
    print $out "   const char* TypeString() { return \"$tname\"; }\n";
    print $out "};\n\n";
}

sub deserialize {
    # xxx A key assumption is that all pointer types are AutoSerializable
    #     All base types can only be references or actual pass-by-value
    #     Other types must have a serializable constructor.

    my ($out, $t, $n, $pkt) = @_;
    my $b = typeBase($t);
    if (typeIsPtr($t)) {
	print $out "$n = CreateObject<$b>($pkt);\n";
    } elsif ($b eq 'int'   || $b eq 'long' || $b eq 'uint32' ||
	     $b =~ /Type$/) { # xxx hack -- assume these are enums
	print $out "$n = ($t)$pkt\->ReadInt();\n";
    } elsif ($b eq 'short' || $b eq 'uint16') {
	print $out "$n = ($t)$pkt\->ReadShort();\n";
    } elsif ($b eq 'float' || $b eq 'double') {
	print $out "$n = ($t)$pkt\->ReadFloat();\n";
    } elsif ($b eq 'bool'  || $b eq 'byte') {
	print $out "$n = ($t)$pkt\->ReadByte();\n";
    } elsif ($b eq 'string') {
	print $out "$pkt\->ReadString(this->$n);\n";
    } else {
	print $out "$n = $b($pkt);\n";
    }
}

sub serialize {
    my ($out, $t, $n, $pkt) = @_;
    my $b = typeBase($t);
    if (typeIsPtr($t)) {
	print $out "$n\->Serialize($pkt);\n";
    } elsif ($b eq 'int'   || $b eq 'long' || $b eq 'uint32' ||
	     $b =~ /Type$/) { # xxx hack -- assume these are enums
	print $out "$pkt\->WriteInt((uint32)$n);\n";
    } elsif ($b eq 'short' || $b eq 'uint16') {
	print $out "$pkt\->WriteShort((uint16)$n);\n";
    } elsif ($b eq 'float' || $b eq 'double') {
	print $out "$pkt\->WriteFloat((float)$n);\n";
    } elsif ($b eq 'bool'  || $b eq 'byte') {
	print $out "$pkt\->WriteByte((byte)$n);\n";
    } elsif ($b eq 'string') {
	print $out "$pkt\->WriteString($n);\n";
    } else {
	print $out "$n.Serialize($pkt);\n";
    }
}

sub getlength {
    my ($out, $t, $n, $len) = @_;
    my $b = typeBase($t);
    if (typeIsPtr($t)) {
	print $out "$len += $n\->GetLength();\n";
    } elsif ($b eq 'int'   || $b eq 'long' || $b eq 'uint32' ||
	     $b =~ /Type$/) { # xxx hack -- assume these are enums
	print $out "$len += 4;\n";
    } elsif ($b eq 'short' || $b eq 'uint16') {
	print $out "$len += 2;\n";
    } elsif ($b eq 'float' || $b eq 'double') {
	print $out "$len += 4;\n";
    } elsif ($b eq 'bool'  || $b eq 'byte') {
	print $out "$len += 1;\n";
    } elsif ($b eq 'string') {
	print $out "$len += 4 + $n.length();\n";
    } else {
	print $out "$len += $n.GetLength();\n";
    }
}

sub typeDefaultVal {
    my $t = shift;
    my $b = typeBase($t);
    if ($t eq 'void') {
	return '';
    } elsif (typeIsPtr($t)) {
	return "NULL";
    } elsif ($b eq 'int'   || $b eq 'long' || $b eq 'uint32' ||
	     $b =~ /Type$/) { # xxx hack -- assume these are enums
	return "(($t)0)";
    } elsif ($b eq 'short' || $b eq 'uint16') {
	return "(($t)0)";
    } elsif ($b eq 'float' || $b eq 'double') {
	return "(($t)0.0)";
    } elsif ($b eq 'bool'  || $b eq 'byte') {
	return "(($t)0)";
    } elsif ($b eq 'string') {
	return "string(\"\")";
    } else {
	return "$b()";
    }
}

###############################################################################

###############################################################################

make_rpc();
