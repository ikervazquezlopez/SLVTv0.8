import java.io.BufferedReader;
import java.util.ArrayList;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import weka.classifiers.functions.MultilayerPerceptron;
import weka.core.Attribute;
import weka.core.DenseInstance;
import weka.core.Instances;

public class WekaModelTest {

	public static void main(String[] args) {
		double[] d = {2,2};
		String s = classifyInstance(d);
		System.out.println("Value: " + s);
	}
	
	
	public static String classifyInstance(double[] d){
		double x1 = d[0];
		double x2 = d[1];
		String value = null;
		String path = "neural_network.model";
		MultilayerPerceptron ann;
		try {
			ann = (MultilayerPerceptron) weka.core.SerializationHelper.read(path);
		
			ArrayList<Attribute> att = new ArrayList<Attribute>(2);
			ArrayList<String> classVal = new ArrayList<String>();
			
			//Define and set the class values
			classVal.add("Zero");
			classVal.add("One");
			classVal.add("Two");
			classVal.add("Three");
			classVal.add("Four");
			classVal.add("Five");
			
			//Define the attributes
			for(int i=1; i<=90000; i++)
				att.add(new Attribute("V" + i));
			att.add(new Attribute("Class", classVal));
			
			Instances dataRaw = new Instances("Data", att, 0);
			
			//Create instance and add it
			double[] instanceValue = new double[dataRaw.numAttributes()];
			for(int i=0; i<90000; i++)
				instanceValue[i] = d[i];
			dataRaw.add(new DenseInstance(1.0,instanceValue));
			
			
			int cIdx = dataRaw.numAttributes() - 1;
			dataRaw.setClassIndex(cIdx);
			
			for(int i=0; i<dataRaw.size(); i++){
				double label = ann.classifyInstance(dataRaw.instance(i));
				//double[] label = ann.distributionForInstance(unlabeled.instance(0));
				dataRaw.instance(i).setClassValue(label);
				double c = dataRaw.instance(i).classValue();
			}
			
			value = classVal.get((int)dataRaw.instance(0).classValue());
			
			
		} 	
		catch (FileNotFoundException e) {	e.printStackTrace();} 
		catch (IOException e) {e.printStackTrace();}
		catch (Exception e) {e.printStackTrace();}
		
		return value;
	}
	
	
	
}
