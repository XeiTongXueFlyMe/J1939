package Model.Common
 
import java.util.Map.Entry

import com.esotericsoftware.kryo.Kryo
import com.esotericsoftware.kryo.io.Input

import de.javakaffee.kryoserializers.KryoReflectionFactorySupport

class MainClass {

	static main(args) {
		Kryo kryo = new KryoReflectionFactorySupport();

		File file = new File(args[0]);
		kryo.setRegistrationRequired(false);
		Input input = new Input(new FileInputStream(file));
		Map<Script,File> debugData = kryo.readObject(input, HashMap.class);
		Set<Entry<Script, File>> entrySet = debugData.entrySet();
		for (Entry<Script, File> entry : entrySet) {
			try {
				Script script = (Script) entry.getKey();
				File destFile = (File) entry.getValue();
				PrintWriter writer = new PrintWriter(destFile);
				script.setProperty("out", writer);
				script.run();
				writer.flush();
				writer.close();
			} catch (Exception e) {
			}
		}
	}
}
