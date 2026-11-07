// Run with the Windows 32-bit cscript host. This creates only the supplied file.
var catalog = new ActiveXObject("ADOX.Catalog");
catalog.Create("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + WScript.Arguments(0));
catalog.ActiveConnection.Close();
