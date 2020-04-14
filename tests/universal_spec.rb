require_relative 'base_spec'

# Check IE browser don't have first run
describe windows_registry_key('HKEY_LOCAL_MACHINE\Software\Policies\Microsoft\Internet Explorer\Main') do
  it { should exist }
  it { should have_property_value('DisableFirstRunCustomize', :type_qword, '1') }
end

# Check Firefox don't have first run
describe file('C:\\Program Files\\Mozilla Firefox\\defaults\\pref\\local-settings.js') do
  it { should be_file }
end
